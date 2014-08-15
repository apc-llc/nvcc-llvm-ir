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
#include <list>
#include <string>
#include <sstream>
#include <vector>

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

static Module* initial_module = NULL;

// Starting form the specified block, follow all braches of parallel region,
// marking target blocks as parallel. Continue until returning back into
// block marked as parallel, or until the end_parallel_region call is approached.
// However, the only valid stopping condition is end_parallel_region, which
// is indicated by "true" return value.
static bool followParallelBasicBlock(BasicBlock* bb, list<BasicBlock*>& pbl, int nparallel)
{
	bool result = false;

	for (BasicBlock::iterator ii = bb->begin(), ie = bb->end(); ii != ie; ii++)
	{
		CallInst* ci = dyn_cast<CallInst>(cast<Value>(ii));
		if (ci)
		{
			Function *callee = dyn_cast<Function>(
				ci->getCalledValue()->stripPointerCasts());
			if (!callee) continue;
			if (callee->getName() == "begin_parallel_region")
			{
				fprintf(stderr, "nvcc-llvm-ir: nested parallel regions are not supported\n");
				exit(1);
			}
			if (callee->getName() == "end_parallel_region")
			{
				// Move CallInst and all insts below CallInst to a new block.
				BasicBlock *nb1 = NULL;
				{
					BasicBlock::iterator SplitIt = ii;
					while (isa<PHINode>(SplitIt) || isa<LandingPadInst>(SplitIt))
						SplitIt++;
					stringstream name;
					name << ".end_parallel_" << nparallel;
					nb1 = bb->splitBasicBlock(SplitIt, bb->getName() + name.str());
				}

				// Nuke end_parallel_region call.
				nb1->begin()->eraseFromParent();
				
				// The end of parallel region has been found - leave now.
				return true;
			}
		}
		
		// Follow successors in BranchInst, SwitchInst and IndirectBranchInst.
		// Skip blocks that are already known to belong to parallel region.
		BranchInst* bi = dyn_cast<BranchInst>(cast<Value>(ii));
		if (bi)
		{
			for (int i = 0, e = bi->getNumSuccessors(); i != e; i++)
			{
				BasicBlock* succ = bi->getSuccessor(i);
				if (find(pbl.begin(), pbl.end(), succ) != pbl.end()) continue;
				pbl.push_back(succ);
				result |= followParallelBasicBlock(succ, pbl, nparallel);
			}
		}
		SwitchInst* si = dyn_cast<SwitchInst>(cast<Value>(ii));
		if (si)
		{
			for (int i = 0, e = si->getNumSuccessors(); i != e; i++)
			{
				BasicBlock* succ = si->getSuccessor(i);
				if (find(pbl.begin(), pbl.end(), succ) != pbl.end()) continue;
				pbl.push_back(succ);
				result |= followParallelBasicBlock(succ, pbl, nparallel);
			}			
		}
		IndirectBrInst* ibi = dyn_cast<IndirectBrInst>(cast<Value>(ii));
		if (ibi)
		{
			for (int i = 0, e = ibi->getNumSuccessors(); i != e; i++)
			{
				BasicBlock* succ = ibi->getSuccessor(i);
				if (find(pbl.begin(), pbl.end(), succ) != pbl.end()) continue;
				pbl.push_back(succ);
				result |= followParallelBasicBlock(succ, pbl, nparallel);
			}
		}
	}
	
	return result;
}

// Mark basic blocks that belong to parallel regions.
static void markParallelBasicBlocks(Module* module, vector<BasicBlock*>& parallelBlocks)
{
	Function* begin_parallel_region = module->getFunction("begin_parallel_region");
	Function* end_parallel_region = module->getFunction("end_parallel_region");
	
	// If parallel region guards are not declared, then they are not used
	// anywhere => no parallel regions, nothing to do, leave early.
	if (!begin_parallel_region && !end_parallel_region)
		return;
	
	if (!begin_parallel_region)
	{
		fprintf(stderr, "nvcc-llvm-ir: unmatched end_parallel_region found\n");
		exit(1);
	}
	if (!end_parallel_region)
	{
		fprintf(stderr, "nvcc-llvm-ir: unmatched begin_parallel_region found\n");
		exit(1);
	}

	list<BasicBlock*> pbl;

	// 1) Split basic blocks at calls to begin_/end_no_predicate_region.
	// 2) Mark basic blocks that belong to loop regions.
	for (Module::iterator fi = module->begin(), fe = module->end(); fi != fe; fi++)
	{
		int nparallel = 0;
		BasicBlock* restart = NULL;
		do
		{
			for (Function::iterator bi = fi->begin(), be = fi->end(); bi != be; bi++)
			{
				BasicBlock* b = bi;
				if (restart && (b != restart)) continue;

				restart = NULL;
				
				// Skip blocks that are already known to belong to parallel region.
				if (find(pbl.begin(), pbl.end(), b) != pbl.end()) continue;				
		
				for (BasicBlock::iterator ii = b->begin(), ie = b->end(); ii != ie; ii++)
				{
					CallInst* ci = dyn_cast<CallInst>(cast<Value>(ii));
					if (!ci) continue;
					Function *callee = dyn_cast<Function>(
						ci->getCalledValue()->stripPointerCasts());
					if (!callee) continue;
					if (callee->getName() != "begin_parallel_region")
						continue;
					
					// Move CallInst and all insts below CallInst to a new block.
					BasicBlock *nb1 = NULL;
					{
						BasicBlock::iterator SplitIt = ii;
						while (isa<PHINode>(SplitIt) || isa<LandingPadInst>(SplitIt))
							SplitIt++;
						stringstream name;
						name << ".begin_parallel_" << nparallel;
						nb1 = bi->splitBasicBlock(SplitIt, b->getName() + name.str());
					}
				
					// Nuke begin_parallel_region call.
					nb1->begin()->eraseFromParent();
					
					// Add nb1 to the list of parallel blocks.
					pbl.push_back(nb1);
					
					// Starting form nb1, follow all braches of parallel region, marking target
					// blocks as parallel. Continue until returning back into block marked as
					// parallel, or until the end_parallel_region call is approached.
					// However, the only valid stopping condition is end_parallel_region, which
					// is indicated by "true" return value.
					if (!followParallelBasicBlock(nb1, pbl, nparallel))
					{
						fprintf(stderr, "nvcc-llvm-ir: unmatched begin_parallel_region found\n");
						exit(1);
					}
					
					nparallel++;

					// Continue iterating basic blocks from nb2.
					restart = nb1;
					break;
				}
			
				if (restart) break;
			}
		}
		while (restart);
	}
	
	// Export parallel blocks list into vector.
	parallelBlocks.reserve(pbl.size());
	parallelBlocks.assign(pbl.begin(), pbl.end());

	// Remove parallel region marks declarations.
	begin_parallel_region->eraseFromParent();
	end_parallel_region->eraseFromParent();
}

// Perform store instructions in threadIdx.x = 0 only.
static void storeInZeroThreadOnly(Module* module, vector<BasicBlock*>& parallelBlocks)
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

				restart = NULL;
				
				// Skip basic blocks belonging to parallel regions.
				if (find(parallelBlocks.begin(), parallelBlocks.end(), b) != parallelBlocks.end())
					continue;
		
				for (BasicBlock::iterator ii = b->begin(), ie = b->end(); ii != ie; ii++)
				{
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
}

static void modifyModule(Module* module)
{
	vector<BasicBlock*> parallelBlocks;

	markParallelBasicBlocks(module, parallelBlocks);

	// Perform store instructions in threadIdx.x = 0 only.
	storeInZeroThreadOnly(module, parallelBlocks);

	outs() << *module << "\n";
}

static bool called_compile = false;

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

		modifyModule(initial_module);

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

		modifyModule(optimized_module);
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

