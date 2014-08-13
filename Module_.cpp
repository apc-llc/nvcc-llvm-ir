#include <llvm/Module.h>
#include <llvm/LLVMContext.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_ostream.h>
#include <nvvm.h>

/*#include <llvm/Support/system_error.h>
#include <llvm/Support/PatternMatch.h>
#include <llvm/Support/GraphWriter.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Endian.h>
#include <llvm/Support/BlockFrequency.h>
#include <llvm/Support/FileUtilities.h>
#include <llvm/Support/Errno.h>
#include <llvm/Support/SwapByteOrder.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/Timer.h>
#include <llvm/Support/IRBuilder.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Support/ConstantRange.h>
#include <llvm/Support/Process.h>
#include <llvm/Support/ToolOutputFile.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/type_traits.h>
#include <llvm/Support/Valgrind.h>
#include <llvm/Support/AlignOf.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/SMLoc.h>
#include <llvm/Support/Regex.h>
#include <llvm/Support/CFG.h>
#include <llvm/Support/NoFolder.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/TypeBuilder.h>
#include <llvm/Support/COFF.h>
#include <llvm/Support/Signals.h>
#include <llvm/Support/MachO.h>
#include <llvm/Support/CrashRecoveryContext.h>
#include <llvm/Support/PluginLoader.h>
#include <llvm/Support/Allocator.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/AIXDataTypesFix.h>
#include <llvm/Support/Format.h>
#include <llvm/Support/DataTypes.h>
#include <llvm/Support/InstIterator.h>
#include <llvm/Support/Win64EH.h>
#include <llvm/Support/DataExtractor.h>
#include <llvm/Support/Dwarf.h>
#include <llvm/Support/LeakDetector.h>
#include <llvm/Support/RWMutex.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/BranchProbability.h>
#include <llvm/Support/MathExtras.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/GCOV.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/ConstantFolder.h>
#include <llvm/Support/DebugLoc.h>
#include <llvm/Support/PathV2.h>
#include <llvm/Support/Threading.h>
#include <llvm/Support/CallSite.h>
#include <llvm/Support/RecyclingAllocator.h>
#include <llvm/Support/GetElementPtrTypeIterator.h>
#include <llvm/Support/DOTGraphTraits.h>
#include <llvm/Support/TargetFolder.h>
#include <llvm/Support/DataFlow.h>
#include <llvm/Support/StringPool.h>
#include <llvm/Support/PrettyStackTrace.h>
#include <llvm/Support/CodeGen.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Support/Atomic.h>
#include <llvm/Support/Mutex.h>
#include <llvm/Support/ELF.h>
#include <llvm/Support/PassNameParser.h>
#include <llvm/Support/PathV1.h>
#include <llvm/Support/MutexGuard.h>
#include <llvm/Support/TimeValue.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/OutputBuffer.h>
#include <llvm/Support/FEnv.h>
#include <llvm/Support/circular_raw_ostream.h>
#include <llvm/Support/IncludeFile.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/Recycler.h>
#include <llvm/Support/Memory.h>
#include <llvm/Support/InstVisitor.h>
#include <llvm/Support/ThreadLocal.h>
#include <llvm/Support/SystemUtils.h>
#include <llvm/Support/PointerLikeTypeTraits.h>
#include <llvm/Support/ValueHandle.h>
#include <llvm/Support/Disassembler.h>
#include <llvm/Support/RegistryParser.h>
#include <llvm/Support/PredIteratorCache.h>
#include <llvm/Support/Compiler.h>
#include <llvm/Support/Registry.h>
#include <llvm/Support/Capacity.h>
#include <llvm/Support/Program.h>
#include <llvm/Support/MemoryObject.h>
#include <llvm/Support/IRReader.h>
#include <llvm/Value.h>
#include <llvm/LinkAllVMCore.h>
#include <llvm/Pass.h>
#include <llvm/InstrTypes.h>
#include <llvm/Constant.h>
#include <llvm/InlineAsm.h>
#include <llvm/CallingConv.h>
#include <llvm/IntrinsicInst.h>
#include <llvm/ValueSymbolTable.h>
#include <llvm/Use.h>
#include <llvm/MC/SectionKind.h>
#include <llvm/MC/SubtargetFeature.h>
#include <llvm/MC/MCExpr.h>
#include <llvm/MC/MCTargetAsmLexer.h>
#include <llvm/MC/MCRegisterInfo.h>
#include <llvm/MC/MCInstrDesc.h>
#include <llvm/MC/MCAsmBackend.h>
#include <llvm/MC/MCLabel.h>
#include <llvm/MC/MCInstrItineraries.h>
#include <llvm/MC/EDInstInfo.h>
#include <llvm/MC/MCWin64EH.h>
#include <llvm/MC/MCInstrAnalysis.h>
#include <llvm/MC/MCAsmLayout.h>
#include <llvm/MC/MCFixup.h>
#include <llvm/MC/MCSectionMachO.h>
#include <llvm/MC/MCAtom.h>
#include <llvm/MC/MCAsmInfoCOFF.h>
#include <llvm/MC/MCSymbol.h>
#include <llvm/MC/MCValue.h>
#include <llvm/MC/MCSubtargetInfo.h>
#include <llvm/MC/MCObjectWriter.h>
#include <llvm/MC/MCSection.h>
#include <llvm/MC/MCELFSymbolFlags.h>
#include <llvm/MC/MCAsmInfo.h>
#include <llvm/MC/MCAssembler.h>
#include <llvm/MC/MachineLocation.h>
#include <llvm/MC/MCFixupKindInfo.h>
#include <llvm/MC/MCDirectives.h>
#include <llvm/MC/MCObjectFileInfo.h>
#include <llvm/MC/MCELFObjectWriter.h>
#include <llvm/MC/MCStreamer.h>
#include <llvm/MC/MCInstrInfo.h>
#include <llvm/MC/MCParser/MCAsmLexer.h>
#include <llvm/MC/MCParser/AsmCond.h>
#include <llvm/MC/MCParser/MCAsmParserExtension.h>
#include <llvm/MC/MCParser/MCAsmParser.h>
#include <llvm/MC/MCParser/MCParsedAsmOperand.h>
#include <llvm/MC/MCParser/AsmLexer.h>
#include <llvm/MC/MCDwarf.h>
#include <llvm/MC/MCCodeGenInfo.h>
#include <llvm/MC/MCDisassembler.h>
#include <llvm/MC/MCTargetAsmParser.h>
#include <llvm/MC/MCContext.h>
#include <llvm/MC/MCInst.h>
#include <llvm/MC/MCSectionELF.h>
#include <llvm/MC/MCInstPrinter.h>
#include <llvm/MC/MCSectionCOFF.h>
#include <llvm/MC/MCAsmInfoDarwin.h>
#include <llvm/MC/MCObjectStreamer.h>
#include <llvm/MC/MCMachObjectWriter.h>
#include <llvm/MC/MCCodeEmitter.h>
#include <llvm/Instruction.h>
#include <llvm/GlobalAlias.h>
#include <llvm/User.h>
#include <llvm/Metadata.h>
#include <llvm/ExecutionEngine/JITEventListener.h>
#include <llvm/ExecutionEngine/Interpreter.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/ExecutionEngine/RuntimeDyld.h>
#include <llvm/ExecutionEngine/JITMemoryManager.h>
#include <llvm/Object/COFF.h>
#include <llvm/Object/MachO.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Object/MachOFormat.h>
#include <llvm/Object/Binary.h>
#include <llvm/Object/MachOObject.h>
#include <llvm/Object/Archive.h>
#include <llvm/Object/Error.h>
#include <llvm/GlobalVariable.h>
#include <llvm/GVMaterializer.h>
#include <llvm/DebugInfoProbe.h>
#include <llvm/Bitcode/BitstreamReader.h>
#include <llvm/Bitcode/LLVMBitCodes.h>
#include <llvm/Bitcode/BitCodes.h>
#include <llvm/Bitcode/BitstreamWriter.h>
#include <llvm/Bitcode/Archive.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Operator.h>
#include <llvm/DefaultPasses.h>
#include <llvm/AutoUpgrade.h>
#include <llvm/PassManager.h>
#include <llvm/Instructions.h>
#include <llvm/GlobalValue.h>
#include <llvm/OperandTraits.h>
#include <llvm/Constants.h>
#include <llvm/Intrinsics.h>
#include <llvm/PassRegistry.h>
#include <llvm/LLVMContext.h>
#include <llvm/Analysis/LibCallAliasAnalysis.h>
#include <llvm/Analysis/CodeMetrics.h>
#include <llvm/Analysis/ConstantFolding.h>
#include <llvm/Analysis/InlineCost.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/IntervalIterator.h>
#include <llvm/Analysis/RegionInfo.h>
#include <llvm/Analysis/DomPrinter.h>
#include <llvm/Analysis/ProfileInfoTypes.h>
#include <llvm/Analysis/InstructionSimplify.h>
#include <llvm/Analysis/MemoryBuiltins.h>
#include <llvm/Analysis/ProfileInfoLoader.h>
#include <llvm/Analysis/Interval.h>
#include <llvm/Analysis/DominatorInternals.h>
#include <llvm/Analysis/LoopIterator.h>
#include <llvm/Analysis/Trace.h>
#include <llvm/Analysis/LoopDependenceAnalysis.h>
#include <llvm/Analysis/RegionPass.h>
#include <llvm/Analysis/Lint.h>
#include <llvm/Analysis/IntervalPartition.h>
#include <llvm/Analysis/DIBuilder.h>
#include <llvm/Analysis/LazyValueInfo.h>
#include <llvm/Analysis/ValueTracking.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Analysis/BlockFrequencyImpl.h>
#include <llvm/Analysis/Loads.h>
#include <llvm/Analysis/PostDominators.h>
#include <llvm/Analysis/PathNumbering.h>
#include <llvm/Analysis/ConstantsScanner.h>
#include <llvm/Analysis/DebugInfo.h>
#include <llvm/Analysis/RegionPrinter.h>
#include <llvm/Analysis/MemoryDependenceAnalysis.h>
#include <llvm/Analysis/ProfileInfo.h>
#include <llvm/Analysis/AliasSetTracker.h>
#include <llvm/Analysis/CaptureTracking.h>
#include <llvm/Analysis/Dominators.h>
#include <llvm/Analysis/Passes.h>
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/Analysis/ScalarEvolutionNormalization.h>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/Analysis/FindUsedTypes.h>
#include <llvm/Analysis/BlockFrequencyInfo.h>
#include <llvm/Analysis/ScalarEvolutionExpander.h>
#include <llvm/Analysis/SparsePropagation.h>
#include <llvm/Analysis/BranchProbabilityInfo.h>
#include <llvm/Analysis/PathProfileInfo.h>
#include <llvm/Analysis/RegionIterator.h>
#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Analysis/PHITransAddr.h>
#include <llvm/Analysis/CFGPrinter.h>
#include <llvm/Analysis/LoopPass.h>
#include <llvm/Analysis/ScalarEvolutionExpressions.h>
#include <llvm/Analysis/DominanceFrontier.h>
#include <llvm/Analysis/LibCallSemantics.h>
#include <llvm/Analysis/DOTGraphTraitsPass.h>
#include <llvm/Analysis/IVUsers.h>
#include <llvm/TableGen/Record.h>
#include <llvm/TableGen/Main.h>
#include <llvm/TableGen/TableGenAction.h>
#include <llvm/TableGen/TableGenBackend.h>
#include <llvm/TableGen/Error.h>
#include <llvm/Target/TargetRegisterInfo.h>
#include <llvm/Target/TargetIntrinsicInfo.h>
#include <llvm/Target/Mangler.h>
#include <llvm/Target/TargetInstrInfo.h>
#include <llvm/Target/TargetLoweringObjectFile.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetJITInfo.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Target/TargetELFWriterInfo.h>
#include <llvm/Target/TargetLowering.h>
#include <llvm/Target/TargetData.h>
#include <llvm/Target/TargetLibraryInfo.h>
#include <llvm/Target/TargetFrameLowering.h>
#include <llvm/Target/TargetSelectionDAGInfo.h>
#include <llvm/Target/TargetSubtargetInfo.h>
#include <llvm/Target/TargetCallingConv.h>
#include <llvm/Target/TargetOpcodes.h>
#include <llvm/Config/config.h>
#include <llvm/Config/llvm-config.h>
#include <llvm/DebugInfo/DIContext.h>
#include <llvm/PassSupport.h>
#include <llvm/Type.h>
#include <llvm/Module.h>
#include <llvm/Argument.h>
#include <llvm/PassManagers.h>
#include <llvm/ADT/SCCIterator.h>
#include <llvm/ADT/EquivalenceClasses.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/SparseBitVector.h>
#include <llvm/ADT/SmallSet.h>
#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/DenseMapInfo.h>
#include <llvm/ADT/StringMap.h>
#include <llvm/ADT/IntervalMap.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/DeltaAlgorithm.h>
#include <llvm/ADT/ImmutableList.h>
#include <llvm/ADT/PointerUnion.h>
#include <llvm/ADT/StringExtras.h>
#include <llvm/ADT/PackedVector.h>
#include <llvm/ADT/BitVector.h>
#include <llvm/ADT/ilist.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/PriorityQueue.h>
#include <llvm/ADT/PostOrderIterator.h>
#include <llvm/ADT/IndexedMap.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/Statistic.h>
#include <llvm/ADT/ValueMap.h>
#include <llvm/ADT/SmallBitVector.h>
#include <llvm/ADT/APSInt.h>
#include <llvm/ADT/ImmutableMap.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/ilist_node.h>
#include <llvm/ADT/StringSet.h>
#include <llvm/ADT/FoldingSet.h>
#include <llvm/ADT/VectorExtras.h>
#include <llvm/ADT/Optional.h>
#include <llvm/ADT/Twine.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/DepthFirstIterator.h>
#include <llvm/ADT/ScopedHashTable.h>
#include <llvm/ADT/APInt.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/Triple.h>
#include <llvm/ADT/SetOperations.h>
#include <llvm/ADT/DenseSet.h>
#include <llvm/ADT/DAGDeltaAlgorithm.h>
#include <llvm/ADT/IntEqClasses.h>
#include <llvm/ADT/PointerIntPair.h>
#include <llvm/ADT/StringSwitch.h>
#include <llvm/ADT/SetVector.h>
#include <llvm/ADT/IntrusiveRefCntPtr.h>
#include <llvm/ADT/GraphTraits.h>
#include <llvm/ADT/TinyPtrVector.h>
#include <llvm/ADT/Trie.h>
#include <llvm/ADT/OwningPtr.h>
#include <llvm/ADT/InMemoryStruct.h>
#include <llvm/ADT/ImmutableSet.h>
#include <llvm/ADT/NullablePtr.h>
#include <llvm/ADT/UniqueVector.h>
#include <llvm/CodeGen/MachORelocation.h>
#include <llvm/CodeGen/RegisterScavenging.h>
#include <llvm/CodeGen/GCs.h>
#include <llvm/CodeGen/MachineMemOperand.h>
#include <llvm/CodeGen/FunctionLoweringInfo.h>
#include <llvm/CodeGen/ScheduleHazardRecognizer.h>
#include <llvm/CodeGen/MachineFunctionAnalysis.h>
#include <llvm/CodeGen/EdgeBundles.h>
#include <llvm/CodeGen/MachineRegisterInfo.h>
#include <llvm/CodeGen/TargetLoweringObjectFileImpl.h>
#include <llvm/CodeGen/GCMetadata.h>
#include <llvm/CodeGen/LiveVariables.h>
#include <llvm/CodeGen/MachineLoopRanges.h>
#include <llvm/CodeGen/RuntimeLibcalls.h>
#include <llvm/CodeGen/LatencyPriorityQueue.h>
#include <llvm/CodeGen/MachineBranchProbabilityInfo.h>
#include <llvm/CodeGen/IntrinsicLowering.h>
#include <llvm/CodeGen/MachineRelocation.h>
#include <llvm/CodeGen/RegAllocPBQP.h>
#include <llvm/CodeGen/SelectionDAGNodes.h>
#include <llvm/CodeGen/LiveStackAnalysis.h>
#include <llvm/CodeGen/JITCodeEmitter.h>
#include <llvm/CodeGen/MachineConstantPool.h>
#include <llvm/CodeGen/MachineDominators.h>
#include <llvm/CodeGen/MachineFunctionPass.h>
#include <llvm/CodeGen/CallingConvLower.h>
#include <llvm/CodeGen/SelectionDAG.h>
#include <llvm/CodeGen/MachineCodeEmitter.h>
#include <llvm/CodeGen/SlotIndexes.h>
#include <llvm/CodeGen/LexicalScopes.h>
#include <llvm/CodeGen/MachineInstrBuilder.h>
#include <llvm/CodeGen/MachineBasicBlock.h>
#include <llvm/CodeGen/ProcessImplicitDefs.h>
#include <llvm/CodeGen/ObjectCodeEmitter.h>
#include <llvm/CodeGen/MachineBlockFrequencyInfo.h>
#include <llvm/CodeGen/Passes.h>
#include <llvm/CodeGen/MachineLoopInfo.h>
#include <llvm/CodeGen/RegAllocRegistry.h>
#include <llvm/CodeGen/BinaryObject.h>
#include <llvm/CodeGen/MachineJumpTableInfo.h>
#include <llvm/CodeGen/MachineCodeInfo.h>
#include <llvm/CodeGen/PBQP/HeuristicBase.h>
#include <llvm/CodeGen/PBQP/Heuristics/Briggs.h>
#include <llvm/CodeGen/PBQP/Math.h>
#include <llvm/CodeGen/PBQP/HeuristicSolver.h>
#include <llvm/CodeGen/PBQP/Graph.h>
#include <llvm/CodeGen/PBQP/Solution.h>
#include <llvm/CodeGen/ScoreboardHazardRecognizer.h>
#include <llvm/CodeGen/MachinePassRegistry.h>
#include <llvm/CodeGen/CalcSpillWeights.h>
#include <llvm/CodeGen/LiveIntervalAnalysis.h>
#include <llvm/CodeGen/LinkAllCodegenComponents.h>
#include <llvm/CodeGen/SelectionDAGISel.h>
#include <llvm/CodeGen/Analysis.h>
#include <llvm/CodeGen/AsmPrinter.h>
#include <llvm/CodeGen/MachineSSAUpdater.h>
#include <llvm/CodeGen/MachineModuleInfo.h>
#include <llvm/CodeGen/PseudoSourceValue.h>
#include <llvm/CodeGen/ISDOpcodes.h>
#include <llvm/CodeGen/FastISel.h>
#include <llvm/CodeGen/LiveInterval.h>
#include <llvm/CodeGen/ScheduleDAG.h>
#include <llvm/CodeGen/MachineModuleInfoImpls.h>
#include <llvm/CodeGen/MachineFunction.h>
#include <llvm/CodeGen/MachineOperand.h>
#include <llvm/CodeGen/LinkAllAsmWriterComponents.h>
#include <llvm/CodeGen/SchedulerRegistry.h>
#include <llvm/CodeGen/ValueTypes.h>
#include <llvm/CodeGen/MachineInstr.h>
#include <llvm/CodeGen/GCStrategy.h>
#include <llvm/CodeGen/GCMetadataPrinter.h>
#include <llvm/CodeGen/MachineFrameInfo.h>
#include <llvm/SymbolTableListTraits.h>
#include <llvm/Function.h>
#include <llvm/DerivedTypes.h>
#include <llvm/PassAnalysisSupport.h>
#include <llvm/Linker.h>
#include <llvm/InitializePasses.h>
#include <llvm/Transforms/Instrumentation.h>
#include <llvm/Transforms/Utils/SSAUpdater.h>
#include <llvm/Transforms/Utils/UnrollLoop.h>
#include <llvm/Transforms/Utils/Local.h>
#include <llvm/Transforms/Utils/BasicInliner.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Transforms/Utils/SimplifyIndVar.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/Utils/FunctionUtils.h>
#include <llvm/Transforms/Utils/AddrModeMatcher.h>
#include <llvm/Transforms/Utils/BuildLibCalls.h>
#include <llvm/Transforms/Utils/ValueMapper.h>
#include <llvm/Transforms/Utils/UnifyFunctionExitNodes.h>
#include <llvm/Transforms/Utils/SSAUpdaterImpl.h>
#include <llvm/Transforms/Utils/PromoteMemToReg.h>
#include <llvm/Transforms/IPO/InlinerPass.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/CallGraphSCCPass.h>
#include <llvm/BasicBlock.h>
#include <llvm/Attributes.h>
#include <llvm/LinkAllPasses.h>
#include <llvm/Assembly/PrintModulePass.h>
#include <llvm/Assembly/Writer.h>
#include <llvm/Assembly/AssemblyAnnotationWriter.h>
#include <llvm/Assembly/Parser.h>*/

#include <cstdio>
#include <dlfcn.h>
#include <iostream>
#include <string>

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

const static bool printFirstModuleOnly = true;

void storeInZeroThreadOnly(Module* module)
{
}

nvvmResult nvvmAddModuleToProgram(nvvmProgram prog, const char *bitcode, size_t size, const char *name)
{
	static bool firstModule = true;

	// Load module from bitcode.
	string source = "";
	source.reserve(size);
	source.assign(bitcode, bitcode + size);
	MemoryBuffer *input = MemoryBuffer::getMemBuffer(source);
	string err;
	LLVMContext &context = getGlobalContext();
	Module* module = ParseBitcodeFile(input, context, &err);
	if (!module)
    	cerr << "Error parsing module bitcode : " << err;
	if (firstModule || !printFirstModuleOnly)
		outs() << *module;
	firstModule = false;

	// TODO: we need to optimize the IR first, and modify it only afterwards!

	// Modify module: perform store instructions in threadIdx.x = 0 only.
	storeInZeroThreadOnly(module);
	
	// Save module into bitcode.
    SmallVector<char, 128> output;
    raw_svector_ostream outputStream(output);
    WriteBitcodeToFile(module, outputStream);
    outputStream.flush();
	
	// Call real nvvmAddModuleToProgram
	bind_lib(LIBNVVM);
	bind_sym(libnvvm, nvvmAddModuleToProgram, nvvmResult, nvvmProgram, const char*, size_t, const char*);
	
	return nvvmAddModuleToProgram_real(prog, output.data(), output.size(), name);
}

bool called_compile = false;

static Module* module_malloc = NULL;

nvvmResult nvvmCompileProgram(nvvmProgram prog, int numOptions, const char **options)
{	
	called_compile = true;

	bind_lib(LIBNVVM);
	bind_sym(libnvvm, nvvmCompileProgram, nvvmResult, nvvmProgram, int, const char**);
	
	cout << "Entering nvvmCompileProgram" << endl;
	
	nvvmResult result = nvvmCompileProgram_real(prog, numOptions, options);

	cout << "Leaving nvvmCompileProgram" << endl;
	
	return result;
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

#define STRING_SEARCH "-globaldce" //"-" //-opt-use-fast-math -R FAST_RELAXED_MATH=1"

char llvmbc[] = { (char)66, (char)67, (char)-64, '\0' };

#define MEMCPY_SEARCH (char*)llvmbc

void print_module(char* src, size_t size)
{
	string source = "";
	source.reserve(size);
	source.assign((char*)src, (char*)src + size);
	MemoryBuffer *input = MemoryBuffer::getMemBuffer(source);
	string err;
	LLVMContext &context = getGlobalContext();
	Module* module = ParseBitcodeFile(input, context, &err);
	if (!module)
		cerr << "Error parsing module bitcode : " << err;
	else	
		outs() << *module;
}

/*void memcpy_signal() { }

extern "C" int strcmp(const char* str1, const char* str2)
{
	bind_lib(LIBC);
	bind_sym(libc, strcmp, int, const char*, const char*);
	
	static bool inside_strcmp = false;
	
	if (!inside_strcmp)
	{
		inside_strcmp = true;
//		printf("strcmp(%s, %s)\n", str1, str2);
//		cout << "strcmp(" << str1 << ", " << str2 << ");" << endl;
		inside_strcmp = false;
	}
	
	return strcmp_real(str1, str2);
}*/

/*#include <vector>

extern "C" int strncmp(const char* str1, const char* str2, size_t size)
{
	bind_lib(LIBC);
	bind_sym(libc, strncmp, int, const char*, const char*, size_t);
	
	static bool inside_strcmp = false;
	
	if (!inside_strcmp)
	{
		inside_strcmp = true;
		vector<char> str1_(str1, str1 + size + 1);
		vector<char> str2_(str2, str2 + size + 1);
		str1_[size] = '\0';
		str2_[size] = '\0';
		printf("strncmp(%s, %s, %zu)\n", &str1_[0], &str2_[0], size);
//		cout << "strcmp(" << str1 << ", " << str2 << ");" << endl;
		inside_strcmp = false;
	}
	
	return strncmp_real(str1, str2, size);
}*/

/*extern "C" void* memcpy(void* dst, const void* src, size_t size)
{
	bind_lib(LIBC);
	bind_sym(libc, memcpy, void*, void*, const void*, size_t);

	static bool inside_memcpy = false;
		
	if (!inside_memcpy)
	{
		inside_memcpy = true;

		if (!strncmp((const char*)src, MEMCPY_SEARCH, strlen(MEMCPY_SEARCH)))
		{
			cout << (const char*)src << size << " @ " << src << endl;
			fflush(stdout);
			memcpy_signal();
		}

		if (size == sizeof(Module))
		{
			printf("memcpy(%p, %p, %zu);\n", dst, src, size);
		}

		inside_memcpy = false;
	}	

	void* result = memcpy_real(dst, src, size);
	
	return result;
}*/

#include <map>

#define SZPOOL ((size_t)1024*1024*8192)
#define SZCHUNKS (1024*1024)

static char* pool = NULL;
static char* chunks[SZCHUNKS];
int nchunks = 0;

#include <pthread.h>
#include <unistd.h>

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static bool inside_free = false;

extern "C" void* malloc(size_t size)
{
	if (!pool)
		pool = (char*)sbrk(SZPOOL);

	if (!size) return NULL;

	static char* poolptr = pool;

	pthread_mutex_lock(&mutex);

	poolptr += 8 - (size_t)poolptr % 4;
	poolptr += 4 - sizeof(size_t);
	*(size_t*)poolptr = size;
	poolptr += sizeof(size_t);
	if (!inside_free && called_compile) chunks[nchunks++] = poolptr;
	void* result = (void*)poolptr;
	poolptr += size;

	if (poolptr >= pool + SZPOOL)
	{
		printf("Memory pool is full!\n");
		abort();
	}
	if (nchunks >= SZCHUNKS)
	{
		printf("Memory chunks index is full!\n");
		abort();
	}

	if (called_compile && !module_malloc)
	{
		if (size == sizeof(Module))
		{
			printf("malloc(%zu) = %p\n", size, result);
			module_malloc = (Module*)result;
		}
	}

	pthread_mutex_unlock(&mutex);

	return result;
}

extern "C" void* calloc(size_t num, size_t size)
{
	void* result = malloc(num * size);
	if (result)
		memset(result, 0, num * size);
	return result;
}

extern "C" void* realloc(void* ptr, size_t size)
{
	if (!size) return NULL;

	if (ptr)
	{
		size_t old_size = *((size_t*)ptr - 1);
		if (old_size >= size)
			return ptr;
		void* result = malloc(size);
		if (result)
			memcpy(result, ptr, old_size);
		return result;
	}
	return malloc(size);
}

extern "C" void free(void* ptr1)
{
	if (ptr1 == module_malloc)
	{
//		module_malloc->dump();
	}	

	/*static int old_nchunks = 0;

	static bool free_analysis_enabled = false;
	if (nchunks >= 114200) free_analysis_enabled = true;

	const char* datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v32:32:32-v64:64:64-v128:128:128-n16:32:64";
	
	const char* triple = "nvptx-nvidia-cl.1.0";

	if (called_compile && !inside_free && free_analysis_enabled && (nchunks != old_nchunks))
	{
		printf("nchunks = %d\n", nchunks);

		inside_free = true;
		
		for (size_t ichunk = 0; ichunk < nchunks; ichunk++)
		{
			char* ptr1 = chunks[ichunk];
			if (!ptr1) continue;
			size_t size = *((size_t*)ptr1 - 1);
		
			for (int ii = 0, ee = size - strlen(triple); ii < ee; ii++)
				if (!strncmp((char*)ptr1 + ii, triple, strlen(triple)))
				{
					char* address1 = (char*)ptr1 + ii;
//					printf("Found triple @ %p + %d = %p\n", ptr1, ii, address1);
					for (size_t ichunk = 0; ichunk < nchunks; ichunk++)
					{
						char* ptr2 = chunks[ichunk];
						size_t size = *((size_t*)ptr2 - 1);
					
						for (int jj = 0, je = size - sizeof(char*); jj < je; jj++)
							if (*(char**)(ptr2 + jj) == address1)
							{
								char* address2 = (char*)ptr2 + jj;
//								printf("Found string using triple @ %p + %d = %p\n", ptr2, jj, address2);
//								cout << *(string*)address2 << endl;
								Module* module = (Module*)ptr2;
								Function* function = module->getFunction("kernel");
								if (function)
								{
									try
									{
										printf("&module = %p\n", module);
										function->dump();
									}
									catch (...)
									{
										printf("Exception during function->dump()\n");
									}
								}
							}
					}
				}
		}

		inside_free = false;
	}
	
	old_nchunks = nchunks;*/
}

