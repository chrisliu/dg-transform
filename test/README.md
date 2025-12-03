# test/

## Directory Functionality and Key Concepts

Test infrastructure for validating libdragongem instrumentation passes, analysis passes, and runtime library functionality.

### Test Coverage

**Current Tests**:
- **llvm/trace-simple/**: Basic validation of instrumentation passes
  - Tests InstrumentSimpointPass and InstrumentInstTracePass
  - Verifies runtime library integration
  - Validates protobuf output generation

**Status**: Minimal test coverage

**Needed Tests** (future work):
- **Instrumentation correctness**: Verify instrumented code produces correct traces
- **Runtime API**: Unit tests for Instrumentation.cpp API functions
- **CanonicalId stability**: Verify ID consistency across compilations
- **Analysis passes**: Validate MemoryAlias and MemoryCheckpoint correctness
- **Protobuf serialization**: Round-trip tests for all schema types
- **Integration tests**: End-to-end compilation → execution → trace validation

### Test Infrastructure

**Test Organization**:
- Tests organized by component (llvm/, trace/, analysis/)
- Each test directory contains:
  - Input files (.ll, .c, .cpp)
  - Expected outputs (.proto, .txt)
  - Test scripts (run.sh, validate.py)

**Running Tests**:
```bash
cd test/llvm/trace-simple
./run.sh  # Execute test and validate output
```

**Test Dependencies**:
- LLVM opt tool (for pass testing)
- Compiler toolchain (clang++, linker)
- libdragongem libraries (passes, runtime)
- Protobuf tools (protoc, for validation)

## File Descriptions

None currently (test infrastructure is minimal)

## Subdirectory Overview

- **llvm/**: LLVM pass and utility tests
  - **trace-simple/**: Basic instrumentation pass validation
