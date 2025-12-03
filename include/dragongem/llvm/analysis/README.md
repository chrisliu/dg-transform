# analysis/

## Directory Functionality and Key Concepts

Public API headers for advanced analysis passes. These headers define data structures and interfaces for memory dependency analysis and program region partitioning.

### APIs

**MemoryAlias.h**:
- **Purpose**: Memory alias analysis API
- **Structures**:
  - `AliasInfo`: Represents alias relationship between two memory operations
    - Fields: `RAW` (read-after-write), `WAR` (write-after-read), `WAW` (write-after-write)
    - Methods: `hasRAW()`, `hasWAR()`, `hasWAW()`, `mustAlias()`, `mayAlias()`
  - `MemoryAliasAnalysis`: Analysis pass interface
    - Methods: `getAliasInfo(Inst1, Inst2)`, `computeAliases(Function*)`
- **Usage**: Dataflow accelerator queries alias info for speculative memory execution
- **Implementation**: Uses LLVM's MemorySSA (see `src/llvm/analysis/MemoryAlias.cpp`)
- **Example**:
  ```cpp
  MemoryAliasAnalysis MAA;
  AliasInfo info = MAA.getAliasInfo(load, store);
  if (info.hasRAW()) { /* enforce dependency */ }
  ```

**MemoryCheckpoint.h**:
- **Purpose**: Program region partitioning API for checkpointing
- **Structures**:
  - `ProgramRegion`: Represents a program region suitable for checkpointing
    - Fields: `entry` (BasicBlock*), `exits` (vector<BasicBlock*>), `cost` (double)
    - Methods: `contains(BasicBlock*)`, `getCheckpointCost()`, `isNested()`
  - `MemoryCheckpointSolver`: Analysis pass interface
    - Methods: `computeRegions(Function*)`, `getOptimalCheckpoints()`
- **Usage**: Checkpointing systems query regions for snapshot placement
- **Implementation**: Analyzes control flow and memory dependencies (see `src/llvm/analysis/MemoryCheckpoint.cpp`)
- **Example**:
  ```cpp
  MemoryCheckpointSolver MCS;
  auto regions = MCS.computeRegions(F);
  for (ProgramRegion& R : regions) { /* place checkpoint at R.entry */ }
  ```

### Design Principles

- **Separation of concerns**: Headers define interface, implementations in `src/llvm/analysis/`
- **LLVM integration**: Follows LLVM pass API conventions
- **Dataflow support**: Designed for hybrid execution models with speculation
- **Incremental computation**: Analyses can be queried on-demand or precomputed

## File Descriptions

- **MemoryAlias.h**: Memory alias analysis API with AliasInfo and MemoryAliasAnalysis structures
- **MemoryCheckpoint.h**: Program region partitioning API with ProgramRegion and MemoryCheckpointSolver structures

## Subdirectory Overview

None (all header files at this level)
