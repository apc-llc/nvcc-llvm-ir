ARCH := 30

NVCC_LLVM_IR := LD_PRELOAD=./libnvcc.so

all: test

test: test.cu
	CICC_MODIFY_OPT_MODULE=1 $(NVCC_LLVM_IR) nvcc -I. -arch=sm_$(ARCH) $< -o $@ >test.ll

clean:
	rm -rf test test.ll
