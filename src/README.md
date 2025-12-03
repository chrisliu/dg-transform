# src/

## Directory Functionality and Key Concepts

Implementation of all libdragongem components: core utilities, instrumentation passes, runtime library, and protobuf message definitions.

### Build Targets

The CMakeLists.txt defines five main library targets:

**dragongem** (umbrella target):
- Meta-target aggregating all library components
- Use for projects requiring full libdragongem functionality

**dragongemLLVM** (core utilities):
- CanonicalId: Persistent ID system for LLVM IR entities
- ExecutableBasicBlock: Executable instruction filtering
- AnalysisManager: LLVM analysis infrastructure wrapper
- Metadata: Static program analysis utilities
- **Dependencies**: LLVM libraries

**dragongemInstrumentationPasses** (LLVM passes):
- InstrumentSimpointPass: Basic block profiling
- InstrumentInstTracePass: Instruction execution tracing
- MetadataPass: Static analysis pass
- AnalyzeAAPass: Alias analysis debugging pass
- Registry.cpp: LLVM pass plugin registration
- CLOpts.cpp: Command-line option definitions
- **Dependencies**: dragongemLLVM, LLVM libraries

**dragongemInstrumentationTools_shared** (runtime library):
- Instrumentation.cpp: Runtime API implementation
- Linked with instrumented binaries to collect traces
- **Dependencies**: dragongemTrace, Protobuf, stream
- **Build**: Shared library for dynamic linking

**dragongemTrace** (protobuf messages):
- BBInterval.proto: Basic block interval frequencies
- InstTrace.proto: Instruction trace events
- LLVMUID.proto: Canonical ID mappings
- Metadata.proto: Static analysis results
- SimStats.proto: Simulation statistics
- **Dependencies**: Protobuf
- **Build**: Auto-generated C++ code via protobuf_generate()

### Dependency Chain

```
dragongemInstrumentationPasses
    ↓
dragongemLLVM
    ↓
LLVM libraries

dragongemInstrumentationTools_shared
    ↓
dragongemTrace
    ↓
Protobuf + stream
```

## File Descriptions

- **CMakeLists.txt**: Build configuration defining all library targets, managing dependencies, setting compiler flags

## Subdirectory Overview

- [llvm/](./llvm/README.md): Core LLVM utilities and instrumentation implementation
- [trace/](./trace/README.md): Protocol Buffer message schemas for data serialization
