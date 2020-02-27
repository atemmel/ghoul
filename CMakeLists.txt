cmake_minimum_required(VERSION 3.5)

project(ghoul)

file(GLOB_RECURSE SOURCES RELATIVE ${CMAKE_SOURCE_DIR} "src/*.cpp")
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
include_directories(include)
add_definitions(${LLVM_DEFINITIONS})

add_executable(ghoul ${SOURCES})
set_property(TARGET ghoul PROPERTY CXX_STANDARD 17)
add_compile_options(-Wall -Wextra -Wpedantic)
target_link_libraries(ghoul LLVM)
