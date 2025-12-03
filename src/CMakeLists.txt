# ============================================================================
# Umbrella Target
# ============================================================================

# Umbrella library that links all dragonGem components
# Note: Built from dummy.cpp for historical reasons
add_library(dragongem SHARED dummy.cpp)

# ============================================================================
# Subdirectories
# ============================================================================

# Protobuf message definitions
add_subdirectory(trace)

# LLVM-based libraries (core utilities, analysis, instrumentation)
add_subdirectory(llvm)

# ============================================================================
# Umbrella Target Dependencies
# ============================================================================

# Link umbrella target to all major components
# This preserves backward compatibility for projects that link against 'dragongem'
target_link_libraries(dragongem
  PUBLIC
    dragongemLLVM
    dragongemAnalysis
    dragongemInstrumentationPasses
)

# ============================================================================
# Installation
# ============================================================================

install(TARGETS dragongem)
