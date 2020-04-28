cmake_minimum_required(VERSION 3.5)

project(ghoul)

# Set a default build type if none was specified
set(default_build_type "Release")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}")
endif()

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

include(GNUInstallDirs)
install(TARGETS ghoul DESTINATION ${CMAKE_INSTALL_BINDIR})
