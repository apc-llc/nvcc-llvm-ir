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

// Add suffix to function name, for example.
extern "C" void runModulePass(Module* module)
{
	if (!module) return;

	for (Module::iterator i = module->begin(), e = module->end(); i != e; i++)
		if (!i->isIntrinsic())
			i->setName(i->getName() + "_modified");
}

