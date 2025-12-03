# ============================================================================
# Core LLVM Utilities Library
# ============================================================================

# Core utilities used across all LLVM-based components
# - CanonicalId: Persistent identification for LLVM IR entities
# - ExecutableBasicBlock: Filter executable instructions
# - AnalysisManager: Wrapper for LLVM analysis infrastructure
# - Metadata: Static analysis metadata structures

add_library(
  dragongemLLVM SHARED
  CanonicalId.cpp
  ExecutableBasicBlock.cpp
  AnalysisManager.cpp
  Metadata.cpp
)

# ============================================================================
# Compiler Options
# ============================================================================

# LLVM built without RTTI, so we must also disable it
target_compile_options(dragongemLLVM PUBLIC -fno-rtti)

# ============================================================================
# Include Directories
# ============================================================================

# Public headers (dragongem/llvm/*.h)
target_include_directories(dragongemLLVM
  PUBLIC
    ${dragongem_INCLUDE_DIRS}
)

# Generated protobuf headers
target_include_directories(dragongemLLVM
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../trace/include>
)

# Stream headers (private, for internal use)
target_include_directories(dragongemLLVM
  PRIVATE
    ${stream_INCLUDE_DIRS}
)

# ============================================================================
# Link Dependencies
# ============================================================================

# Depends on protobuf trace messages
target_link_libraries(dragongemLLVM PUBLIC dragongemTrace)

# ============================================================================
# Subdirectories
# ============================================================================

# Analysis passes library
add_subdirectory(analysis)

# Instrumentation passes and runtime library
add_subdirectory(trace)

# ============================================================================
# Installation
# ============================================================================

install(TARGETS dragongemLLVM)
