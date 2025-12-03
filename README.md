# libdragongem/

## Directory Functionality and Key Concepts

LLVM out-of-tree library providing instrumentation passes, analysis passes, runtime infrastructure, and core utilities for DragonGem's trace-driven simulation framework.

### Core Components

**Instrumentation Passes** (`src/llvm/trace/`):
- **InstrumentSimpointPass**: Basic block profiling for phase detection
- **InstrumentInstTracePass**: Detailed instruction-level execution tracing
- **MetadataPass**: Static program analysis (function/loop classification)

**Analysis Passes** (`src/llvm/analysis/`):
- **MemoryAliasAnalysis**: Memory dependency analysis via MemorySSA
- **MemoryCheckpointAnalysis**: Program region partitioning for checkpointing

**Runtime Library** (`src/llvm/trace/Instrumentation.cpp`):
- Trace collection infrastructure linked with instrumented programs
- Implements API called by instrumented code
- Generates protobuf trace files consumed by gem5

**Core Utilities** (`src/llvm/`):
- **CanonicalId**: Persistent ID system for LLVM IR entities across compilation/simulation boundary
- **ExecutableBasicBlock**: Filters executable instructions from basic blocks
- **AnalysisManager**: Wrapper for LLVM analysis infrastructure
- **Metadata**: Static analysis metadata structures

### Build System

CMake-based build with external dependencies:
- **LLVM**: Core compiler infrastructure (find_package)
- **Protobuf**: Data serialization (find_package)
- **stream**: Submodule dependency for I/O utilities

**Library Targets**:
- `dragongem`: Umbrella target
- `dragongemLLVM`: Core utilities (CanonicalId, ExecutableBasicBlock, AnalysisManager, Metadata)
- `dragongemInstrumentationPasses`: LLVM passes (SimPoint, InstTrace, Metadata, AnalyzeAA)
- `dragongemInstrumentationTools_shared`: Runtime library
- `dragongemTrace`: Protobuf message definitions and generated code

**Build commands**:
```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### Integration with LLVM

**Using instrumentation passes**:
```bash
# Load passes
opt -load-pass-plugin=build/src/libdragongemInstrumentationPasses.so \
    -passes=dg-inst-trace input.ll -o instrumented.bc

# Link with runtime
clang++ instrumented.bc -ldragongemInstrumentationTools_shared -o program

# Execute with tracing
DG_MODE=inst_trace ./program  # Generates trace file
```

**Trace modes** (controlled by `DG_MODE` environment variable):
- `simpoint`: Basic block frequency profiling
- `inst_trace`: Detailed instruction execution trace

### Data Flow

1. **Static Analysis**: MetadataPass ’ Metadata.proto
2. **Dynamic Profiling**: InstrumentSimpointPass ’ BBInterval.proto
3. **Trace Collection**: InstrumentInstTracePass + Runtime ’ InstTrace.proto
4. **Simulation**: gem5 replays InstTrace.proto for architectural simulation

### Integration Points

- **libdragongem ’ gem5**: InstTrace.proto consumed by `cpu/llvm/trace_replay_engine`
- **libdragongem ’ scripts**: Compilation and trace generation orchestration
- **LLVM IR ’ Canonical IDs**: Persistent identification for cross-boundary communication

## File Descriptions

- **CMakeLists.txt**: Top-level build configuration, defines library targets, manages dependencies (LLVM, Protobuf, stream)
- **.clang-format**: Code formatting rules for C++ source files

## Subdirectory Overview

- [src/](./src/README.md): Implementation of all library components (core, passes, runtime, protobuf)
- [include/dragongem/llvm/](./include/dragongem/llvm/README.md): Public API headers
- **deps/stream/**: External dependency submodule (I/O utilities, documented upstream)
- [test/](./test/README.md): Test cases for instrumentation and analysis
