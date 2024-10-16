#ifndef DRAGONGEM_LLVM_TRACE_INSTRUMENTATION_INTERFACE_H
#define DRAGONGEM_LLVM_TRACE_INSTRUMENTATION_INTERFACE_H

#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Module.h"

namespace dragongem {
namespace llvm {
namespace trace {

struct InstrumentationInterface {
  InstrumentationInterface(::llvm::Module &M);

  ::llvm::IntegerType *I64Ty;
  ::llvm::FunctionCallee IncDynamicInstCountFunc;

  ::llvm::FunctionCallee RecordFunctionEntryFunc;
  ::llvm::FunctionCallee RecordReturnInstFunc;

  ::llvm::FunctionCallee RecordBasicBlockFunc;
  ::llvm::FunctionCallee RecordLandingPadFunc;

  ::llvm::FunctionCallee RecordLoadInstFunc;
  ::llvm::FunctionCallee RecordStoreInstFunc;
};

} // namespace trace
} // namespace llvm
} // namespace dragongem

#endif // DRAGONGEM_LLVM_TRACE_INSTRUMENTATION_INTERFACE_H
