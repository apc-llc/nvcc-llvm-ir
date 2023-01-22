cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

project(nvcc-llvm-ir LANGUAGES CXX CUDA)

find_package(LLVM CONFIG PATHS "/usr/local/lib/cmake" NO_DEFAULT_PATH)

if (LLVM_FOUND)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Locate NVVM components in the CUDA toolkit directory tree:
# https://github.com/nvidia-compiler-sdk/nvvmir-samples/blob/master/CMakeLists.txt
find_package(CUDAToolkit REQUIRED)
get_filename_component(CUDA_HOME "${CUDAToolkit_BIN_DIR}" DIRECTORY)
find_file(LIBNVVM_HOME nvvm PATHS "${CUDA_HOME}")
find_library(NVVM_LIB nvvm PATHS "${LIBNVVM_HOME}/lib64" "${LIBNVVM_HOME}/lib/x64")
find_file(NVVM_H nvvm.h PATH "${LIBNVVM_HOME}/include")
get_filename_component(NVVM_INCLUDE_DIRS ${NVVM_H} DIRECTORY)

add_library(cicc SHARED "src/cicc.cpp")
target_include_directories(cicc PRIVATE ${LLVM_INCLUDE_DIRS})
target_include_directories(cicc PRIVATE ${NVVM_INCLUDE_DIRS})
target_compile_features(cicc PRIVATE cxx_std_17)
if (NOT WIN32)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif()
target_compile_definitions(cicc PRIVATE ${LLVM_DEFINITIONS_LIST})
target_compile_options(cicc PRIVATE $<$<BOOL:${MSVC}>:/wd4624> $<$<BOOL:${MSVC}>:/wd4291> $<$<BOOL:${MSVC}>:/MT>)
target_link_directories(cicc PRIVATE ${LLVM_LIBRARY_DIRS})

# With older LLVM releases, instead of just linking agains ${LLVM_LIBRARIES},
# we have to use this idiotic procedure:
# https://llvm.org/docs/CMake.html#embedding-llvm-in-your-project
# Find the libraries that correspond to the LLVM components we wish to use
llvm_map_components_to_libnames(llvm_libs core support ipo analysis target scalaropts transformutils instcombine)
target_link_libraries(cicc PRIVATE ${llvm_libs})

add_library(nvcc SHARED "src/nvcc.cpp")
target_link_libraries(nvcc ${CMAKE_DL_LIBS})
target_compile_definitions(nvcc PRIVATE LIBCICC="$<TARGET_FILE:cicc>")

add_executable(${PROJECT_NAME} "src/${PROJECT_NAME}.cpp")
target_compile_definitions(${PROJECT_NAME} PRIVATE LIBNVCC="$<TARGET_FILE:nvcc>")

add_custom_target(test DEPENDS test1 test2)
add_custom_target(test1 DEPENDS test1_unopt test1_opt)
add_custom_target(test2 DEPENDS test2_unopt test2_opt)

# Prepend CUDA compiler with a launcher, which shall perform preloading
# of our shared library wrapper.
set(CMAKE_CUDA_COMPILER_LAUNCHER ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME})

add_executable(test1_unopt "src/test1.cu")
add_dependencies(test1_unopt cicc nvcc)
set_property(TARGET test1_unopt PROPERTY CUDA_SEPARABLE_COMPILATION ON)
target_compile_options(test1_unopt PRIVATE -keep --${PROJECT_NAME}-unopt)

add_executable(test1_opt "src/test1.cu")
add_dependencies(test1_unopt cicc nvcc)
set_property(TARGET test1_opt PROPERTY CUDA_SEPARABLE_COMPILATION ON)
target_compile_options(test1_opt PRIVATE -keep --${PROJECT_NAME}-opt)

add_executable(test2_unopt "src/test2.cu")
add_dependencies(test2_unopt cicc nvcc)
set_property(TARGET test2_unopt PROPERTY CUDA_SEPARABLE_COMPILATION ON)
target_compile_options(test2_unopt PRIVATE -keep --${PROJECT_NAME}-unopt)

add_executable(test2_opt "src/test2.cu")
add_dependencies(test2_opt cicc nvcc)
set_property(TARGET test2_opt PROPERTY CUDA_SEPARABLE_COMPILATION ON)
target_compile_options(test2_opt PRIVATE -keep --${PROJECT_NAME}-opt)

endif()

add_custom_target(docker
    COMMAND docker build -f ${CMAKE_CURRENT_SOURCE_DIR}/docker/Dockerfile -t ${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/docker
    COMMAND docker run -it --rm --user "${UID}:${GID}" -v${CMAKE_CURRENT_SOURCE_DIR}:/project ${PROJECT_NAME} sh /project/docker/scripts/build.sh
    COMMENT "Building ${PROJECT_NAME} in a Docker container")

