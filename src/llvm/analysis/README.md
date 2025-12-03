# analysis/

## Directory Functionality and Key Concepts

Advanced LLVM analysis passes for memory dependency analysis and program region partitioning.

### Components

**MemoryAlias** (`MemoryAlias.cpp`):
- **Purpose**: Memory dependency analysis using LLVM's MemorySSA
- **Analysis**: Determines RAW, WAR, WAW dependencies between memory operations
- **Approach**: Leverages MemorySSA for precise alias information
- **Usage**: Dataflow accelerator uses alias info for speculative memory execution
- **API**: Provides `AliasInfo` structures for memory instruction pairs
- **Status**: Implementation complete (3,184 bytes)

**MemoryCheckpoint** (`MemoryCheckpoint.cpp`):
- **Purpose**: Program region partitioning for optimal checkpointing placement
- **Analysis**: Identifies program regions where checkpoints minimize rollback cost
- **Approach**: Analyzes control flow and memory dependencies to partition program
- **Usage**: Checkpointing systems use regions for snapshot placement
- **API**: Provides `ProgramRegion` structures with checkpoint boundaries
- **Status**: Minimal stub implementation (274 bytes, work in progress)

### Integration

These analysis passes support hybrid execution models where different program regions execute on different accelerators or with different speculation strategies. The analyses inform:
- **Memory ordering constraints**: Which memory operations must serialize
- **Speculation boundaries**: Where speculation can safely begin/end
- **Checkpoint placement**: Optimal locations for state snapshots

## File Descriptions

- **MemoryAlias.cpp**: MemorySSA-based memory dependency analysis providing alias information (3,184 bytes)
- **MemoryCheckpoint.cpp**: Program region partitioning for checkpointing strategies (274 bytes, stub)

## Subdirectory Overview

None (all implementation files at this level)
