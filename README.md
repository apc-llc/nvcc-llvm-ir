# Enabling on-the-fly manipulations with LLVM IR code of CUDA sources

Largerly thanks to [LLVM](http://llvm.org/), in recent years we've seen a significant increase of interest to domain-specific compilation tools research & development. With the release of PTX backends by NVIDIA (opensource [NVPTX](http://llvm.org/docs/NVPTXUsage.html) and proprietary [libNVVM](https://developer.nvidia.com/cuda-llvm-compiler)), construction of custom LLVM-driven compilers for generating GPU binaries also becomes possible. However, two questions are still remaining:

1. how to customize the CUDA source compilation
2. what is the best set of GPU-specific LLVM optimizations and how to continue modifying IR after applying them

The first question is the result of opensource CUDA *frontend* unavailability. In fact the *EDG* frontend (by Edison Design Group Inc.) used by NVIDIA CUDA compiler is the only frontend that is able to translate CUDA source into LLVM IR. LLVM's clang has basic support for some CUDA constructs, but yet is too far from implementing the entire set of parallel extensions. EDG frontend is tightly bound to the rest of CUDA compiler (*cicc*), and there is no public API to use it just for LLVM IR generation.

The second question is essential for generating efficient GPU code and its further customization. It's well-known that standard LLVM NVPTX backend and NVIDIA's libNVVM may generate different code, because libNVVM applies specific passes in addition to standard `-O3` set. For instance, as of CUDA 6.0, libNVVM has the following optimization chain for `sm_30` target:

```
opt -nv-cuda -nvvm-pretreat -generic-to-nvvm -nv-inline-must -R __CUDA_PREC_DIV=1 -R __CUDA_PREC_SQRT=1 -opt-arch=sm_30 -inline -globaldce -lower-struct-args -memory-space-opt=1 -disable-rsqrt-opt=1 -O3
```

Some of passes mnemonics do not exist in standard LLVM 3.0, meaning they are likely NVIDIA's proprietary extensions. Thus, GPU code generation could not be fully reproduced by opensource NVPTX backend. On the other hand, if libNVVM backend is used, then input LLVM IR is translated directly into PTX code, without possibility to review and modify the optimized IR before PTX generation.

In order to remove this limiations, we have created a special dynamic library. Being attached to NVIDIA CUDA compiler, this library exposes unoptimized and optimized LLVM IR code to the user and allows its on-the-fly modification. As result, domain-specific compiler developer receives flexibility e.g. to retarget CUDA-generated LLVM IR to different architectures, or to make additional modifications to IR after NVIDIA's optimization set. Below we explain the technical details of how unoptimized and optimized LLVM IR versions have been retrieved from CUDA compiler by our dynamic library.

## Unoptimized LLVM IR retrieval

## Optimized LLVM IR retrieval
