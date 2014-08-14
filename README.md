# Enabling on-the-fly manipulations with LLVM IR code of CUDA sources

Largerly thanks to [LLVM](http://llvm.org/), in recent years we've seen a significant increase of interest to domain-specific compilation tools research & development. With the release of PTX backends by NVIDIA (opensource [NVPTX](http://llvm.org/docs/NVPTXUsage.html) and proprietary [libNVVM](https://developer.nvidia.com/cuda-llvm-compiler)), construction of custom LLVM-driven compilers for generating GPU binaries also becomes possible. However, two questions are still remaining:

1. How to customize the CUDA source compilation?
2. What is the best set of GPU-specific LLVM optimizations and how to continue modifying IR after applying them?

The first question is the result of opensource CUDA *frontend* unavailability. In fact the *EDG* frontend (by Edison Design Group Inc.) used by NVIDIA CUDA compiler is the only frontend that is able to translate CUDA source into LLVM IR. LLVM's clang has basic support for some CUDA constructs, but yet is too far from implementing the entire set of parallel extensions. EDG frontend is tightly bound to the rest of CUDA compiler (*cicc*), and there is no public API to use it just for LLVM IR generation.

The second question is essential for generating efficient GPU code and its further customization. It's well-known that standard LLVM NVPTX backend and NVIDIA's libNVVM may generate different code, because libNVVM applies specific passes in addition to standard `-O3` set. For instance, as of CUDA 6.0, libNVVM has the following optimization chain for `sm_30` target:

```
opt -nv-cuda -nvvm-pretreat -generic-to-nvvm -nv-inline-must -R __CUDA_PREC_DIV=1 -R __CUDA_PREC_SQRT=1 -opt-arch=sm_30 -inline -globaldce -lower-struct-args -memory-space-opt=1 -disable-rsqrt-opt=1 -O3
```

Some of passes mnemonics do not exist in standard LLVM 3.0, meaning they are likely NVIDIA's proprietary extensions. Thus, GPU code generation could not be fully reproduced by opensource NVPTX backend. On the other hand, if libNVVM backend is used, then input LLVM IR is translated directly into PTX code, without possibility to review and modify the optimized IR before PTX generation.

In order to remove this limiations, we have created a special dynamic library. Being attached to NVIDIA CUDA compiler, this library exposes unoptimized and optimized LLVM IR code to the user and allows its on-the-fly modification. As result, domain-specific compiler developer receives flexibility e.g. to retarget CUDA-generated LLVM IR to different architectures, or to make additional modifications to IR after NVIDIA's optimization set. Below we explain the technical details of how unoptimized and optimized LLVM IR versions have been retrieved from CUDA compiler by our dynamic library.

## NVIDIA CUDA compiler general review

NVIDIA CUDA compiler is a complex set of pipelined code processing binaries. After the input source is preprocessed and decomposed into separate host and device sources, compiler driver (*nvcc*) deploys CUDA-to-LLVM compiler -- *cicc*, which shall be our main point of interest.

According to the [License For Customer Use of NVIDIA Software](http://www.nvidia.com/content/DriverDownload-March2009/licence.php?lang=us), customer may not reverse engineer, decompile, or disassemble the software, nor attempt in any other manner to obtain the source code. Being in strict compliance with this requirement, we analyzed *cicc* only by means of basic debugging tool and standard C library calls instrumentation.

## Unoptimized LLVM IR retrieval

Retrieval of unoptimized LLVM IR is relatively straight-forward. In order to generate the PTX code, *cicc* deploys libNVVM library functions, which have documented interface. Instrumentation of the first call to `nvvmAddModuleToProgram` function allows to retrieve the LLVM IR for input CUDA source from the second parameter, which is the LLVM bitcode string. This bitcode could be parsed into LLVM Module instance using compatible LLVM 3.0 library functions and printed as IR:

```
string source = "";
source.reserve(size);
source.assign(bitcode, bitcode + size);
MemoryBuffer *input = MemoryBuffer::getMemBuffer(source);
string err;
LLVMContext &context = getGlobalContext();
initial_module = ParseBitcodeFile(input, context, &err);
if (!initial_module)
	cerr << "Error parsing module bitcode : " << err;

outs() << *initial_module;
```

On-the-fly modification of unoptimized LLVM could be archieved by exporting LLVM Module back into bitcode string:

```
SmallVector<char, 128> output;
raw_svector_ostream outputStream(output);
WriteBitcodeToFile(initial_module, outputStream);
outputStream.flush();

// Call real nvvmAddModuleToProgram
return nvvmAddModuleToProgram_real(prog, output.data(), output.size(), name);
```

Note the unoptimized LLVM IR does not include math and GPU-specific builtins, that are linked-in later.

## Optimized LLVM IR retrieval

The libNVVM library itself statically links to NVIDIA's customized LLVM engine, and like most of other binaries in CUDA Toolkit is fully stripped (no debug info, no function frames, etc.). Fortunately, libNVVM is still dynamically linked against the standard C library, which allows to analyse memory allocations and data transfers. Instrumentation of `malloc` function reveals Module-sized space allocation in the beginning of `nvvmCompileProgam`:

```
void* result = malloc_real(size);

if (called_compile)
{
	if (size == sizeof(Module))
		optimized_module = (Module*)result;
}
```

Luckly, this very Module instance exists during entire compilation process, and accumulates all changes made to input LLVM IR by optimization passes. Thus, we only need to find an appropriate moment to intercept this Module and modify its contents. The subsequent call to `localtime` is used as heuristic. Unlike the unoptimized case, this Module could be printed and modified directly, without loading/storing any bitcode.

Retrieved optimized LLVM IR is linked together with math and GPU-specific builtins and is ready for PTX backend.

## Compiling and using nvcc-llvm-ir

Finally, let's demonstrate LLVM IR on-the-fly modification in action. Consider the following input CUDA source:

```
$ cat test.cu
extern "C" __device__ void kernel(int* result) { *result = 1; }
```

Compile our dynamic libraries, specifying include paths to NVVM and LLVM 3.0 headers:

```
$ make
g++ -g -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -I/opt/llvm-3.0/include -I/opt/cuda/nvvm/include/ -fPIC cicc.cpp -shared -o libcicc.so -ldl
g++ -g -I/opt/cuda/nvvm/include/ -fPIC nvcc.cpp -shared -o libnvcc.so -ldl
```

The LLVM IR retrieval mode is specified by two environment variables:

* `CICC_MODIFY_UNOPT_MODULE=1` -- retrieve unoptimized LLVM IR and change it as specified in `modifyModule` function (`cicc.cpp` source file)
* `CICC_MODIFY_OPT_MODULE=1` -- retrieve unoptimized LLVM IR and change it as specified in `modifyModule` function (`cicc.cpp` source file)

Example `modifyModule` simply adds suffix to all existing functions names:

```
void modifyModule(Module* module)
{
	if (!module) return;

	// Add suffix to function name, for example.
	for (Module::iterator i = module->begin(), e = module->end(); i != e; i++)
		i->setName(i->getName() + "_modified");
}

```

Each of the following two commands deploys the corresponding retrieval mode:

```
CICC_MODIFY_UNOPT_MODULE=1 LD_PRELOAD=./libnvcc.so nvcc -arch=sm_30 test.cu -c -keep
CICC_MODIFY_OPT_MODULE=1 LD_PRELOAD=./libnvcc.so nvcc -arch=sm_30 test.cu -c -keep
```

The `-keep` option is added to store the `test.ptx` file, which could be opened to ensure the LLVM IR modification has landed into output PTX code:

```
$ cat test.ptx 
//
// Generated by NVIDIA NVVM Compiler
// Compiler built on Thu Mar 13 19:31:35 2014 (1394735495)
// Cuda compilation tools, release 6.0, V6.0.1
//

.version 4.0
.target sm_30
.address_size 64

.visible .func kernel_modified(
	.param .b64 kernel_modified_param_0
)
{
	.reg .s32 	%r<2>;
	.reg .s64 	%rd<2>;


	ld.param.u64 	%rd1, [kernel_modified_param_0];
	mov.u32 	%r1, 1;
	st.u32 	[%rd1], %r1;
	ret;
}
```

## Final credits

This library has been developed for the purpose of software interoperability and used in compilation of [https://github.com/apc-llc/sixtrack](CERN SixTrack application).

