cmake_minimum_required(VERSION 3.23)

project(libdragongem VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)

if(POLICY CMP0079)
  cmake_policy(SET CMP0079 NEW)
endif()

set(dg_LLVM_INSTALL_DIR
    ""
    CACHE PATH "LLVM installation directory")

if(NOT EXISTS "${dg_LLVM_INSTALL_DIR}")
  message(FATAL_ERROR "Must define dg_LLVM_INSTALL_DIR")
endif()

include(GNUInstallDirs)

find_package(LLVM REQUIRED CONFIG PATHS "${dg_LLVM_INSTALL_DIR}")
include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

set(protobuf_MODULE_COMPATIBLE
    ON
    CACHE BOOL "") # Enable flag for CMake FindPackage
find_package(Protobuf REQUIRED)
include_directories(${Protobuf_INCLUDE_DIRS})

# Include stream directory.
set(stream_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/deps/stream/src/)

set(dragongem_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include/)

add_subdirectory(src)

install(
  DIRECTORY ${stream_INCLUDE_DIRS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/stream
  FILES_MATCHING
  PATTERN "*.h*")
