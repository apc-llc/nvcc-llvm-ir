#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <nvvm.h>

#include <dlfcn.h>
#include <filesystem>
#include <iostream>
#include <cstdio>
#include <list>
#include <string>
#include <sstream>
#include <vector>

using namespace llvm;
using namespace std;

namespace fs = std::filesystem;

#define LIBNVVM "libnvvm.so"

static void* libnvvm = NULL;

#define bind_lib(lib) \
if (!libnvvm) \
{ \
	libnvvm = dlopen(lib, RTLD_NOW | RTLD_GLOBAL); \
	if (!libnvvm) \
	{ \
		fprintf(stderr, "Error loading %s: %s\n", lib, dlerror()); \
		abort(); \
	} \
}

#define bind_sym(handle, sym, retty, ...) \
typedef retty (*sym##_func_t)(__VA_ARGS__); \
static sym##_func_t sym##_real = NULL; \
if (!sym##_real) \
{ \
	sym##_real = (sym##_func_t)dlsym(handle, #sym); \
	if (!sym##_real) \
	{ \
		fprintf(stderr, "Error loading %s: %s\n", #sym, dlerror()); \
		abort(); \
	} \
}

static Module* initial_module = NULL;

static bool called_compile = false;

typedef void (*RunModulePassFunc)(Module* m);

// Load the user-defined module pass from the specified shared library file.
static RunModulePassFunc* getModulePass(std::string filename)
{
	// Module pass file must exist.
	fs::path p(filename);
	std::error_code ec; // For noexcept overload usage.
	if (!fs::exists(p, ec) || !ec)
		return nullptr;

	// Module pass must be readable.
	auto perms = fs::status(p, ec).permissions();
	if ((perms & fs::perms::owner_read) == fs::perms::none ||
		(perms & fs::perms::group_read) == fs::perms::none ||
		(perms & fs::perms::others_read) == fs::perms::none)
		return nullptr;

	void* handle = dlopen(filename.c_str(), RTLD_NOW);
	if (!handle) return nullptr;

	RunModulePassFunc* runModulePass = (RunModulePassFunc*)dlsym(handle, "runModulePass");
	if (!runModulePass) return nullptr;

	return runModulePass;
}

nvvmResult nvvmAddModuleToProgram(nvvmProgram prog, const char *bitcode, size_t size, const char *name)
{
	bind_lib(LIBNVVM);
	bind_sym(libnvvm, nvvmAddModuleToProgram, nvvmResult, nvvmProgram, const char*, size_t, const char*);

	// Load module from bitcode.
	const char* filename = getenv("CICC_MODIFY_UNOPT_MODULE");
	if (filename && !initial_module)
	{
		auto runModulePass = getModulePass(filename);
		if (runModulePass)
		{
			string source = "";
			source.reserve(size);
			source.assign(bitcode, bitcode + size);
			auto input = MemoryBuffer::getMemBuffer(source);
			LLVMContext context;
			auto m = parseBitcodeFile(input.get()->getMemBufferRef(), context);
			initial_module = m.get().get();
			if (!initial_module)
				cerr << "Error parsing module bitcode" << endl;

			(*runModulePass)(initial_module);

			// Save module back into bitcode.
			SmallVector<char, 128> output;
			raw_svector_ostream outputStream(output);
			WriteBitcodeToFile(*initial_module, outputStream);

			// Call real nvvmAddModuleToProgram
			return nvvmAddModuleToProgram_real(prog, output.data(), output.size(), name);
	
		}
	}

	called_compile = true;

	// Call real nvvmAddModuleToProgram
	return nvvmAddModuleToProgram_real(prog, bitcode, size, name);	
}

#undef bind_lib

#define LIBC "libc.so.6"

static void* libc = NULL;

#define bind_lib(lib) \
if (!libc) \
{ \
	libc = dlopen(lib, RTLD_NOW | RTLD_GLOBAL); \
	if (!libc) \
	{ \
		fprintf(stderr, "Error loading %s: %s\n", lib, dlerror()); \
		abort(); \
	} \
}

static Module* optimized_module = NULL;

struct tm *localtime(const time_t *timep)
{
	static bool localtime_first_call = true;

	bind_lib(LIBC);
	bind_sym(libc, localtime, struct tm*, const time_t*);

	const char* filename = getenv("CICC_MODIFY_OPT_MODULE");
       	if (filename && called_compile && localtime_first_call)
	{
		localtime_first_call = false;

		auto runModulePass = getModulePass(filename);
		if (runModulePass)
			(*runModulePass)(optimized_module);
	}
	
	return localtime_real(timep);
}

#include <unistd.h>

#define MAX_SBRKS 16

struct sbrk_t { void* address; size_t size; };
static sbrk_t sbrks[MAX_SBRKS];
static int nsbrks = 0;

static std::mutex mtx;

extern "C" void* malloc(size_t size)
{
	if (!size) return NULL;

	static bool __thread inside_malloc = false;
	
	if (!inside_malloc)
	{
		inside_malloc = true;

		bind_lib(LIBC);
		bind_sym(libc, malloc, void*, size_t);
		
		inside_malloc = false;

		void* result = malloc_real(size);

		if (called_compile && !optimized_module)
		{
			if (size == sizeof(Module))
				optimized_module = (Module*)result;
		}

		return result;
	}

	void* result = sbrk(size);
	if (nsbrks == MAX_SBRKS)
	{
		fprintf(stderr, "Out of sbrk tracking pool space\n");
		mtx.unlock();
		abort();
	}
	mtx.lock();
	sbrk_t s; s.address = result; s.size = size;
	sbrks[nsbrks++] = s;
	mtx.unlock();

	return result;
}

extern "C" void* realloc(void* ptr, size_t size)
{
	bind_lib(LIBC);
	bind_sym(libc, realloc, void*, void*, size_t);
	
	for (int i = 0; i < nsbrks; i++)
		if (ptr == sbrks[i].address)
		{
			void* result = malloc(size);
#define MIN(a,b) (a) < (b) ? (a) : (b)
			memcpy(result, ptr, MIN(size, sbrks[i].size));
			return result;
		}
	
	return realloc_real(ptr, size);
}

extern "C" void free(void* ptr)
{
	bind_lib(LIBC);
	bind_sym(libc, free, void, void*);

	mtx.lock();
	for (int i = 0; i < nsbrks; i++)
		if (ptr == sbrks[i].address) return;
	mtx.unlock();
	
	free_real(ptr);
}

