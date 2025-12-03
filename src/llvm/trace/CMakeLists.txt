# ============================================================================
# Instrumentation Passes Plugin
# ============================================================================

# LLVM out-of-tree passes for program instrumentation
# - InstrumentSimpointPass: Basic block profiling
# - InstrumentInstTracePass: Detailed instruction tracing
# - MetadataPass: Static program analysis
# - AnalyzeAAPass: Alias analysis debugging
# - TracePass: Generic trace pass
# - Registry: Pass registration for LLVM plugin system
# - CLOpts: Command-line options
# - InstrumentationInterface: Runtime API declarations

add_library(
  dragongemInstrumentationPasses SHARED
  InstrumentSimpointPass.cpp
  InstrumentInstTracePass.cpp
  MetadataPass.cpp
  AnalyzeAAPass.cpp
  TracePass.cpp
  Registry.cpp
  CLOpts.cpp
  InstrumentationInterface.cpp
)

# ============================================================================
# Compiler Options
# ============================================================================

# LLVM built without RTTI
target_compile_options(dragongemInstrumentationPasses PUBLIC -fno-rtti)

# ============================================================================
# Link Dependencies
# ============================================================================

# Depends on core utilities (CanonicalId, ExecutableBasicBlock, etc.)
target_link_libraries(dragongemInstrumentationPasses
  PUBLIC
    dragongemLLVM
)

# ============================================================================
# Installation
# ============================================================================

install(TARGETS dragongemInstrumentationPasses)

# ============================================================================
# Runtime Library (Instrumentation Tools)
# ============================================================================

# Shared library implementing runtime API called by instrumented code
# Linked with instrumented programs (not with compiler passes)

add_library(
  dragongemInstrumentationTools SHARED
  Instrumentation.cpp
)

# ============================================================================
# Runtime Library Configuration
# ============================================================================

# LLVM built without RTTI
target_compile_options(dragongemInstrumentationTools PUBLIC -fno-rtti)

# Include paths for runtime
target_include_directories(dragongemInstrumentationTools
  PUBLIC
    ${dragongem_INCLUDE_DIRS}
)

# Generated protobuf headers (for trace serialization)
target_include_directories(dragongemInstrumentationTools
  PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../../>
    ${stream_INCLUDE_DIRS}
)

# Runtime library dependencies
target_link_libraries(dragongemInstrumentationTools
  PRIVATE
    dragongemTrace      # For protobuf serialization
  PUBLIC
    dragongemLLVM       # For CanonicalId utilities
)

# ============================================================================
# Installation
# ============================================================================

install(TARGETS dragongemInstrumentationTools)
