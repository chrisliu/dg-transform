# ============================================================================
# Analysis Passes Library
# ============================================================================

# LLVM analysis passes for memory analysis and program structure
# - MemoryAlias: Memory dependency analysis via MemorySSA
# - MemoryCheckpoint: Program region partitioning for checkpointing

add_library(
  dragongemAnalysis SHARED
  MemoryAlias.cpp
  MemoryCheckpoint.cpp
)

# ============================================================================
# Compiler Options
# ============================================================================

# LLVM built without RTTI
target_compile_options(dragongemAnalysis PUBLIC -fno-rtti)

# ============================================================================
# Include Directories
# ============================================================================

# Public headers (dragongem/llvm/analysis/*.h)
target_include_directories(dragongemAnalysis
  PUBLIC
    ${dragongem_INCLUDE_DIRS}
)

# ============================================================================
# Link Dependencies
# ============================================================================

# Analysis passes depend on core utilities
target_link_libraries(dragongemAnalysis
  PUBLIC
    dragongemLLVM
)

# ============================================================================
# Installation
# ============================================================================

install(TARGETS dragongemAnalysis)
