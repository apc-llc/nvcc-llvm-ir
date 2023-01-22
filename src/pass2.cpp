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

#include <iostream>
#include <cstdio>
#include <list>
#include <string>
#include <sstream>
#include <vector>

using namespace llvm;
using namespace std;

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
				BasicBlock* b = &*bi;
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
				BasicBlock* b = &*bi;
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

extern "C" void runModulePass(Module* module)
{
	vector<BasicBlock*> parallelBlocks;

	markParallelBasicBlocks(module, parallelBlocks);

	// Perform store instructions in threadIdx.x = 0 only.
	storeInZeroThreadOnly(module, parallelBlocks);
#if 0
	// Rerunning -O3 optimization after our modifications.
	PassManager manager;
	PassManagerBuilder builder;
	builder.Inliner = 0;
	builder.OptLevel = 3;
	builder.SizeLevel = 3;
	builder.DisableUnrollLoops = true;
	builder.populateModulePassManager(manager);
	manager.run(*module);

	outs() << *module << "\n";
#endif
}

