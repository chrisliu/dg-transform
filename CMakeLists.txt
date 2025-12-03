cmake_minimum_required(VERSION 3.23)

project(libdragongem VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)

if(POLICY CMP0079)
  cmake_policy(SET CMP0079 NEW)
endif()

set(CMAKE_CXX_FLAGS -fdiagnostics-color=always)
set(CMAKE_C_FLAGS -fdiagnostics-color=always)

include(GNUInstallDirs)

# ============================================================================
# LLVM Configuration
# ============================================================================

# Option to specify custom LLVM installation
# Usage: cmake -DDRAGONGEM_LLVM_DIR=/path/to/llvm/install ..
set(DRAGONGEM_LLVM_DIR "" CACHE PATH
    "Path to custom LLVM installation (optional, uses system LLVM if not set)")

if(DRAGONGEM_LLVM_DIR)
  message(STATUS "Using custom LLVM from: ${DRAGONGEM_LLVM_DIR}")
  set(LLVM_DIR "${DRAGONGEM_LLVM_DIR}/lib/cmake/llvm")
  if(NOT EXISTS "${LLVM_DIR}")
    message(FATAL_ERROR
      "LLVM CMake directory not found at ${LLVM_DIR}. "
      "Please verify DRAGONGEM_LLVM_DIR points to a valid LLVM installation.")
  endif()
else()
  message(STATUS "Using system LLVM (find_package)")
endif()

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "LLVM Install Dir: ${LLVM_INSTALL_PREFIX}")
message(STATUS "LLVM Include Dirs: ${LLVM_INCLUDE_DIRS}")

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

# ============================================================================
# Protobuf Configuration
# ============================================================================

set(protobuf_MODULE_COMPATIBLE ON CACHE BOOL
    "Enable flag for CMake FindPackage")
find_package(Protobuf REQUIRED CONFIG)
message(STATUS "Found Protobuf ${Protobuf_VERSION}")
include_directories(${Protobuf_INCLUDE_DIRS})

# ============================================================================
# Project Include Directories
# ============================================================================

# Public headers
set(dragongem_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include/)
# Stream dependency (submodule)
set(stream_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/deps/stream/src/)

# Make these available to all subdirectories
include_directories(${dragongem_INCLUDE_DIRS})
include_directories(${stream_INCLUDE_DIRS})

# ============================================================================
# Build Subdirectories
# ============================================================================

add_subdirectory(src)

# ============================================================================
# Installation Rules
# ============================================================================

# Install stream headers
install(
  DIRECTORY ${stream_INCLUDE_DIRS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/stream
  FILES_MATCHING
  PATTERN "*.h*")

# Install dragongem public headers
install(
  DIRECTORY ${dragongem_INCLUDE_DIRS}/dragongem/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dragongem
  FILES_MATCHING
  PATTERN "*.h*")
