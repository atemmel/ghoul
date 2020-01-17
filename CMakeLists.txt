cmake_minimum_required(VERSION 3.5)

project(lang)

file(GLOB_RECURSE SOURCES RELATIVE ${CMAKE_SOURCE_DIR} "src/*.cpp")
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
include_directories(include)
add_definitions(${LLVM_DEFINITIONS})

#llvm_map_components_to_libnames(llvm_libs all)

add_executable(lang ${SOURCES})
set_property(TARGET lang PROPERTY CXX_STANDARD 17)
add_compile_options(-Wall -Wextra -Wpedantic)
target_link_libraries(lang LLVM)
#target_link_libraries(lang ${llvm_libs})
