set -e -x
cd /project
mkdir -p build-docker
cd build-docker
cmake -DCMAKE_BUILD_TYPE=Release -DLLVM_DIR=/usr/local/lib/cmake/llvm -DCMAKE_CXX_COMPILER=g++-11 .. && \
#cmake --build .
make VERBOSE=1
