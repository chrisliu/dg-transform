# trace/

## Directory Functionality and Key Concepts

LLVM instrumentation passes and runtime library for program tracing. Provides two instrumentation modes: SimPoint profiling (basic block frequencies) and InstTrace (detailed execution traces).

### Instrumentation Passes

**InstrumentSimpointPass** (`InstrumentSimpointPass.cpp`):
- **Purpose**: Basic block execution frequency profiling for phase detection
- **Instrumentation**: Inserts counters at basic block entry points
- **Runtime**: Calls `DRAGONGEM_START_BB(canonical_id)` for each executed block
- **Output**: BBInterval.proto containing frequency intervals
- **Usage**: `opt -passes=dg-inst-simpoint`
- **Application**: Identify program phases for representative sampling

**InstrumentInstTracePass** (`InstrumentInstTracePass.cpp`):
- **Purpose**: Detailed instruction-level execution tracing
- **Instrumentation**: Inserts trace calls for loads, stores, branches, calls, returns
- **Runtime**: Records complete execution trace with operand values, addresses
- **Output**: InstTrace.proto consumed by gem5's trace replay engine
- **Usage**: `opt -passes=dg-inst-trace`
- **Cost**: High overhead, use for detailed simulation

**MetadataPass** (`MetadataPass.cpp`):
- **Purpose**: Static program analysis pass
- **Analysis**: Function classification, loop nesting, call graph structure
- **Output**: Metadata.proto with static analysis results
- **Usage**: `opt -passes=dg-metadata`
- **Integration**: Metadata used by gem5 for trace interpretation

**AnalyzeAAPass** (`AnalyzeAAPass.cpp`):
- **Purpose**: Alias analysis debugging and validation
- **Analysis**: Queries LLVM alias analysis for memory dependencies
- **Output**: Diagnostic information for verifying AA correctness
- **Usage**: `opt -passes=dg-analyze-aa`
- **Development**: Used during development of memory analysis passes

### Runtime Library

**Instrumentation.cpp**:
- **Purpose**: Implements runtime API called by instrumented code
- **API Functions**:
  - `DRAGONGEM_START_BB(id)`: Record basic block execution
  - `DRAGONGEM_LOAD(addr, size, id)`: Record load operation
  - `DRAGONGEM_STORE(addr, size, id)`: Record store operation
  - `DRAGONGEM_BRANCH(taken, id)`: Record branch outcome
  - `DRAGONGEM_CALL(target, id)`: Record function call
  - `DRAGONGEM_RETURN(id)`: Record function return
- **Mode Control**: DG_MODE environment variable selects tracing mode
  - `simpoint`: Basic block profiling only
  - `inst_trace`: Full instruction trace
- **Output**: Writes protobuf files (BBInterval or InstTrace)
- **Linkage**: Shared library linked with instrumented binaries

### Pass Infrastructure

**Registry.cpp**:
- **Purpose**: LLVM pass plugin registration
- **Registration**: Makes passes discoverable via `-load-pass-plugin`
- **Passes**: Registers SimPoint, InstTrace, Metadata, AnalyzeAA passes
- **Plugin API**: Uses LLVM's new pass manager plugin interface

**CLOpts.cpp**:
- **Purpose**: Command-line option definitions
- **Options**: Configuration flags for instrumentation behavior
- **Scope**: Options available when passes are loaded
- **Usage**: Control trace granularity, filtering, output paths

### Instrumentation Workflow

1. **Compilation**:
   ```bash
   opt -load-pass-plugin=libdragongemInstrumentationPasses.so \
       -passes=dg-inst-trace input.ll -o instrumented.bc
   ```

2. **Linking**:
   ```bash
   clang++ instrumented.bc -ldragongemInstrumentationTools_shared -o program
   ```

3. **Execution**:
   ```bash
   DG_MODE=inst_trace ./program  # Generates trace.pb
   ```

4. **Simulation**:
   ```bash
   gem5 --trace=trace.pb  # Replay trace in architectural model
   ```

## File Descriptions

### Instrumentation Passes
- **InstrumentSimpointPass.cpp**: Basic block profiling instrumentation for phase detection
- **InstrumentInstTracePass.cpp**: Detailed instruction trace instrumentation for gem5 replay
- **MetadataPass.cpp**: Static analysis pass for program classification
- **AnalyzeAAPass.cpp**: Alias analysis debugging pass

### Pass Infrastructure
- **Registry.cpp**: LLVM pass plugin registration for discoverable passes
- **CLOpts.cpp**: Command-line option definitions for instrumentation configuration

### Runtime Library
- **Instrumentation.cpp**: Runtime API implementation called by instrumented code (25,802 bytes)
- **Instrumentation.h**: Runtime API declarations

## Subdirectory Overview

None (all implementation files at this level)
