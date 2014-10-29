ARCH := 30

NVCC_LLVM_IR := LD_PRELOAD=./libnvcc.so

all: matrixMul matrixMul_UNOPT matrixMul_OPT

matrixMul: matrixMul.cu
	nvcc -I. -arch=sm_$(ARCH) $< -o $@

matrixMul_UNOPT: matrixMul.cu
	CICC_MODIFY_UNOPT_MODULE=1 $(NVCC_LLVM_IR) nvcc -I. -arch=sm_$(ARCH) $< -o $@

matrixMul_OPT: matrixMul.cu
	CICC_MODIFY_OPT_MODULE=1 $(NVCC_LLVM_IR) nvcc -I. -arch=sm_$(ARCH) $< -o $@

clean:
	rm -rf matrixMul matrixMul_UNOPT matrixMul_OPT

