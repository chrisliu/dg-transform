cmake_minimum_required(VERSION 3.8)

project(libdragongem VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 17)

if(POLICY CMP0079)
  cmake_policy(SET CMP0079 NEW)
endif()

set(DG_LLVM_INSTALL_DIR
    ""
    CACHE PATH "LLVM installation directory")

if(NOT EXISTS "${DG_LLVM_INSTALL_DIR}")
  message(FATAL_ERROR "Must define DG_LLVM_INSTALL_DIR")
endif()

# TODO: hard-coded
find_package(LLVM REQUIRED CONFIG PATHS "${DG_LLVM_INSTALL_DIR}")

message("${LLVM_INCLUDE_DIRS}")
message("${LLVM_DEFINITIONS_LIST}")

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

add_subdirectory(src)
