# trace/

## Directory Functionality and Key Concepts

Protocol Buffer schema definitions for serializing trace data, metadata, and simulation statistics. Auto-generates C++ code for efficient binary serialization.

### Protocol Buffer Schemas

**BBInterval.proto**:
- **Purpose**: Basic block execution frequency intervals
- **Producer**: InstrumentSimpointPass runtime (simpoint mode)
- **Consumer**: Phase detection tools, SimPoint analysis
- **Content**: Sequence of basic block IDs with execution counts per interval
- **Usage**: Identify representative program phases for sampling-based simulation
- **Format**: `repeated BBIntervalEntry { uint64 bb_id, uint64 count }`

**InstTrace.proto**:
- **Purpose**: Detailed instruction execution trace events
- **Producer**: InstrumentInstTracePass runtime (inst_trace mode)
- **Consumer**: gem5's `cpu/llvm/trace_replay_engine`
- **Content**: Sequential trace of loads, stores, branches, calls, returns with:
  - Canonical instruction IDs
  - Memory addresses and sizes
  - Branch outcomes
  - Function call targets
  - Operand values
- **Usage**: Cycle-accurate architectural simulation via trace replay
- **Format**: `repeated TraceEvent { oneof { LoadEvent, StoreEvent, BranchEvent, ... } }`

**LLVMUID.proto**:
- **Purpose**: Canonical ID mappings for LLVM IR entities
- **Producer**: CanonicalId system during compilation
- **Consumer**: gem5 trace interpreter
- **Content**: Mappings from canonical IDs to IR entity metadata:
  - Function IDs → function names, signatures
  - BasicBlock IDs → function + block index
  - Instruction IDs → opcode, operands, source location
- **Usage**: Resolve canonical IDs in traces back to original LLVM IR
- **Format**: `repeated UIDMapping { uint64 id, EntityMetadata metadata }`

**Metadata.proto**:
- **Purpose**: Static program analysis results
- **Producer**: MetadataPass
- **Consumer**: gem5 configuration, analysis tools
- **Content**: Function and loop classifications:
  - Function types (leaf, recursive, external)
  - Loop nesting depth
  - Call graph structure
  - Hot paths and critical sections
- **Usage**: Guide simulation configuration and optimization strategies
- **Format**: `message ProgramMetadata { repeated FunctionInfo, repeated LoopInfo }`

**SimStats.proto**:
- **Purpose**: Simulation statistics and performance metrics
- **Producer**: gem5 during simulation
- **Consumer**: Analysis scripts, visualization tools
- **Content**: Performance counters, cache statistics, execution metrics
- **Usage**: Post-simulation analysis and visualization
- **Format**: `message SimulationStats { map<string, double> counters }`

### Build Process

CMakeLists.txt uses `protobuf_generate()` to auto-generate C++ code:
- Input: `*.proto` schema files
- Output: `*.pb.h` and `*.pb.cc` in build directory
- Integration: Generated code compiled into `dragongemTrace` library
- Dependencies: Requires Protobuf compiler (protoc) and runtime library

**Generated API**:
```cpp
#include "BBInterval.pb.h"
BBInterval interval;
interval.add_entry()->set_bb_id(42);
interval.add_entry()->set_count(1000);
std::ofstream out("trace.pb", std::ios::binary);
interval.SerializeToOstream(&out);
```

## File Descriptions

- **BBInterval.proto**: Basic block frequency intervals for phase detection (SimPoint profiling)
- **InstTrace.proto**: Instruction execution trace events for gem5 architectural simulation
- **LLVMUID.proto**: Canonical ID mappings from stable IDs to LLVM IR entity metadata
- **Metadata.proto**: Static program analysis results (function/loop classification)
- **SimStats.proto**: Simulation statistics and performance counters from gem5
- **CMakeLists.txt**: Build configuration using protobuf_generate() for code generation

## Subdirectory Overview

None (all schema files at this level)
