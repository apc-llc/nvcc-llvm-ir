#include <llvm/Constants.h>
#include <llvm/Module.h>
#include <llvm/LLVMContext.h>
#include <llvm/Instructions.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_ostream.h>
#include <nvvm.h>

#include <dlfcn.h>
#include <iostream>
#include <cstdio>
#include <string>
#include <sstream>

using namespace llvm;
using namespace std;

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

Module* initial_module = NULL;

// Modify module: perform store instructions in threadIdx.x = 0 only.
void storeInZeroThreadOnly(Module* module)
{
	if (!module) return;

	Type* int32Ty = Type::getInt32Ty(module->getContext());
	Value* zero = ConstantInt::get(int32Ty, 0);

	const char* threadIdxName = "llvm.nvvm.read.ptx.sreg.tid.x";
	Function* threadIdx = module->getFunction(threadIdxName);
	if (!threadIdx)
	{
		FunctionType* ft = FunctionType::get(int32Ty, std::vector<Type*>(), false);
		threadIdx = Function::Create(ft, Function::ExternalLinkage, threadIdxName, module);
	}

	// Add suffix to function name, for example.
	for (Module::iterator fi = module->begin(), fe = module->end(); fi != fe; fi++)
	{
		int nsplits = 0;
		BasicBlock* restart = NULL;
		do
		{
			for (Function::iterator bi = fi->begin(), be = fi->end(); bi != be; bi++)
			{
				BasicBlock* b = bi;
				if (restart && (b != restart)) continue;
		
				for (BasicBlock::iterator ii = b->begin(), ie = b->end(); ii != ie; ii++)
				{
					restart = NULL;

					StoreInst* si = dyn_cast<StoreInst>(cast<Value>(ii));
					if (!si) continue;
			
					// Move StoreInst and all insts below StoreInst to a new block.
					BasicBlock *nb1 = NULL;
					{
						BasicBlock::iterator SplitIt = ii;
						while (isa<PHINode>(SplitIt) || isa<LandingPadInst>(SplitIt))
							SplitIt++;
						stringstream name;
						name << ".store_" << nsplits;
						nb1 = bi->splitBasicBlock(SplitIt, b->getName() + name.str());
					}
				
					BasicBlock::iterator nii1 = nb1->begin();
					nii1++;

					// Move all insts below StoreInst to a new block.
					BasicBlock *nb2 = NULL;
					{
						BasicBlock::iterator SplitIt = nii1;
						while (isa<PHINode>(SplitIt) || isa<LandingPadInst>(SplitIt))
							SplitIt++;				
						stringstream name;
						name << ".else_" << nsplits;
						nb2 = nb1->splitBasicBlock(SplitIt, b->getName() + name.str());
					}

					// Call intrinsic to retrieve threadIdx value.
					Value* tid = CallInst::Create(threadIdx, "", b->getTerminator());
				
					// Check if threadIdx is equal to zero. 
					Value* cond = new ICmpInst(b->getTerminator(),
						ICmpInst::ICMP_EQ, tid, zero, "");

					// Nuke the old uncond branch.
					b->getTerminator()->eraseFromParent();

					// Conditionaly branch to nb1 or nb2, depending on threadIdx.
					BranchInst* bi = BranchInst::Create(nb1, nb2, cond, b);

					nsplits++;
				
					// Continue iterating basic blocks from nb2.
					restart = nb2;
					break;
				}
			
				if (restart) break;
			}
		}
		while (restart);
	}

	outs() << *module << "\n";
}

bool called_compile = false;

nvvmResult nvvmAddModuleToProgram(nvvmProgram prog, const char *bitcode, size_t size, const char *name)
{
	bind_lib(LIBNVVM);
	bind_sym(libnvvm, nvvmAddModuleToProgram, nvvmResult, nvvmProgram, const char*, size_t, const char*);

	// Load module from bitcode.
	if (getenv("CICC_MODIFY_UNOPT_MODULE") && !initial_module)
	{
		string source = "";
		source.reserve(size);
		source.assign(bitcode, bitcode + size);
		MemoryBuffer *input = MemoryBuffer::getMemBuffer(source);
		string err;
		LLVMContext &context = getGlobalContext();
		initial_module = ParseBitcodeFile(input, context, &err);
		if (!initial_module)
			cerr << "Error parsing module bitcode : " << err;

		printf("\n===========================\n");
		printf("MODULE BEFORE OPTIMIZATIONS\n");
		printf("===========================\n\n");		

		// Modify module: perform store instructions in threadIdx.x = 0 only.
		storeInZeroThreadOnly(initial_module);

		// Save module back into bitcode.
		SmallVector<char, 128> output;
		raw_svector_ostream outputStream(output);
		WriteBitcodeToFile(initial_module, outputStream);
		outputStream.flush();

		// Call real nvvmAddModuleToProgram
		return nvvmAddModuleToProgram_real(prog, output.data(), output.size(), name);
	}

	called_compile = true;

	// Call real nvvmAddModuleToProgram
	return nvvmAddModuleToProgram_real(prog, bitcode, size, name);	
}

#undef bind_lib
#undef bind_sym

#define LIBC "libc.so.6"

static void* libc = NULL;

#define bind_lib(lib) \
if (!libc) \
{ \
	libc = dlopen(lib, RTLD_NOW | RTLD_GLOBAL); \
	if (!libc) \
	{ \
		cerr << "Error loading " << lib << ": " << dlerror() << endl; \
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
		cerr << "Error loading " << #sym << ": " << dlerror() << endl; \
		abort(); \
	} \
}

static Module* optimized_module = NULL;

struct tm *localtime(const time_t *timep)
{
	static bool localtime_first_call = true;

	bind_lib(LIBC);
	bind_sym(libc, localtime, struct tm*, const time_t*);

	if (getenv("CICC_MODIFY_OPT_MODULE") && called_compile && localtime_first_call)
	{
		localtime_first_call = false;

		printf("\n==========================\n");
		printf("MODULE AFTER OPTIMIZATIONS\n");
		printf("==========================\n\n");

		// Modify module: perform store instructions in threadIdx.x = 0 only.
		storeInZeroThreadOnly(optimized_module);
	}
	
	return localtime_real(timep);
}

#include <unistd.h>

#define MAX_SBRKS 16

struct sbrk_t { void* address; size_t size; };
static sbrk_t sbrks[MAX_SBRKS];
static int nsbrks = 0;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

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
		pthread_mutex_unlock(&mutex);
		abort();
	}
	pthread_mutex_lock(&mutex);
	sbrk_t s; s.address = result; s.size = size;
	sbrks[nsbrks++] = s;
	pthread_mutex_unlock(&mutex);

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

	pthread_mutex_lock(&mutex);
	for (int i = 0; i < nsbrks; i++)
		if (ptr == sbrks[i].address) return;
	pthread_mutex_unlock(&mutex);
	
	free_real(ptr);
}

