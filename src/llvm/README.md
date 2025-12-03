# llvm/

## Directory Functionality and Key Concepts

Core LLVM utilities for IR analysis and transformation, providing fundamental infrastructure used by instrumentation and analysis passes.

### Components

**CanonicalId** (`CanonicalId.cpp`):
- **Purpose**: Persistent identification system for LLVM IR entities
- **Problem**: LLVM pointers (Function*, BasicBlock*, Instruction*) are transient and invalid across compilation/execution boundary
- **Solution**: Assigns stable sequential IDs to IR entities for cross-boundary communication
- **Usage**: gem5 trace replay uses canonical IDs to map dynamic traces back to static IR
- **Serialization**: LLVMUID.proto stores ID mappings

**ExecutableBasicBlock** (`ExecutableBasicBlock.cpp`):
- **Purpose**: Filters executable instructions from basic blocks
- **Filtering**: Excludes debug intrinsics, non-executable metadata, PHI nodes (architecture-specific handling)
- **Usage**: Instrumentation passes use this to instrument only execution-relevant instructions
- **API**: `getExecutableInstructions(BasicBlock*)` returns filtered instruction list

**AnalysisManager** (`AnalysisManager.cpp`):
- **Purpose**: Wrapper for LLVM's analysis infrastructure
- **Functionality**: Manages analysis pass dependencies, caching, and lifecycle
- **Usage**: Passes use this to request LLVM analyses (dominance, alias analysis, loop info)
- **Abstraction**: Isolates passes from LLVM's pass manager API changes

**Metadata** (`Metadata.cpp`):
- **Purpose**: Static program analysis for function and loop classification
- **Analysis Types**:
  - Function categorization (leaf, recursive, external)
  - Loop nesting analysis
  - Call graph traversal
- **Usage**: MetadataPass uses this to generate static analysis results
- **Output**: Metadata.proto contains classification results

## File Descriptions

- **AnalysisManager.cpp**: LLVM analysis infrastructure wrapper for pass dependencies
- **CanonicalId.cpp**: Persistent ID system mapping IR entities to stable sequential IDs
- **ExecutableBasicBlock.cpp**: Executable instruction filtering for instrumentation
- **Metadata.cpp**: Static analysis utilities for function/loop classification

## Subdirectory Overview

- [trace/](./trace/README.md): Instrumentation passes and runtime library implementation
- [analysis/](./analysis/README.md): Advanced analysis passes (memory alias, checkpointing)
