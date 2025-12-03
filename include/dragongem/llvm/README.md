# llvm/

## Directory Functionality and Key Concepts

Public API headers for libdragongem, providing interfaces to core utilities and instrumentation infrastructure. These headers define the stable API for library users.

### Core APIs

**AnalysisManager.h**:
- **Purpose**: LLVM analysis infrastructure wrapper
- **Interface**: Manages analysis pass dependencies and caching
- **Usage**: Passes request LLVM analyses (dominance, loops, alias analysis) via this manager
- **Abstraction**: Isolates users from LLVM pass manager API changes
- **Example**: `analysisManager.getLoopInfo(Function*)`

**CanonicalId.h**:
- **Purpose**: Persistent ID system for LLVM IR entities
- **Interface**: Assigns and queries stable IDs for Functions, BasicBlocks, Instructions
- **Problem Solved**: LLVM pointers are transient across compilation/execution boundary
- **Usage**: Trace generation assigns IDs during compilation, gem5 uses IDs during simulation
- **Example**: `canonicalId.getInstructionId(Inst*)`
- **Serialization**: IDs stored in LLVMUID.proto

**ExecutableBasicBlock.h**:
- **Purpose**: Executable instruction filtering
- **Interface**: `getExecutableInstructions(BasicBlock*)` returns filtered list
- **Filtering**: Excludes debug intrinsics, non-executable metadata, PHI nodes
- **Usage**: Instrumentation passes use this to instrument only execution-relevant instructions
- **Example**: `for (Instruction* I : ExecutableBasicBlock::get(BB))`

**Metadata.h**:
- **Purpose**: Static program analysis metadata structures
- **Interface**: Classes for FunctionInfo, LoopInfo, CallGraphNode
- **Content**: Function classification (leaf, recursive), loop nesting, call graph
- **Usage**: MetadataPass populates these structures, serialized to Metadata.proto
- **Example**: `FunctionInfo::isLeaf()`, `LoopInfo::getNestingDepth()`

### Header Organization

Headers follow LLVM coding standards:
- Include guards: `#ifndef DRAGONGEM_LLVM_<NAME>_H`
- Forward declarations minimize include dependencies
- Implementation details in corresponding `.cpp` files in `src/llvm/`

## File Descriptions

- **AnalysisManager.h**: LLVM analysis infrastructure wrapper for pass dependencies and caching
- **CanonicalId.h**: Persistent ID system mapping IR entities to stable sequential IDs
- **ExecutableBasicBlock.h**: API for filtering executable instructions from basic blocks
- **Metadata.h**: Static analysis metadata structures (FunctionInfo, LoopInfo, CallGraphNode)
- **MemoryAliasAnalysis.h**: Empty placeholder header (0 bytes, deprecated or unused)

## Subdirectory Overview

- [analysis/](./analysis/README.md): Analysis pass API headers (memory alias, checkpointing)
