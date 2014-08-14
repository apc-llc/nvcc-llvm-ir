.PHONY: test test_unopt test_opt

all: libcicc.so libnvcc.so

libcicc.so: cicc.cpp
	g++ -g -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -I/opt/llvm-3.0/include -I/opt/cuda/nvvm/include/ -fPIC $< -shared -o $@ -ldl -L/opt/llvm-3.0/lib -lLLVMCore -lLLVMSupport -lpthread

libnvcc.so: nvcc.cpp
	g++ -g -I/opt/cuda/nvvm/include/ -fPIC $< -shared -o $@ -ldl

clean:
	rm -rf libcicc.so libnvcc.so

test: test_unopt test_opt

test_unopt: libcicc.so libnvcc.so
	CICC_MODIFY_UNOPT_MODULE=1 LD_PRELOAD=./libnvcc.so nvcc -arch=sm_30 test.cu -rdc=true -c -keep

test_opt: libcicc.so libnvcc.so
	CICC_MODIFY_OPT_MODULE=1 LD_PRELOAD=./libnvcc.so nvcc -arch=sm_30 test.cu -rdc=true -c -keep

