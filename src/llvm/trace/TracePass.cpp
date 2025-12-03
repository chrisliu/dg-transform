#include "TracePass.h"
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Support/Debug.h>

#define DEBUG_TYPE "dg-trace"

namespace dragongem {
namespace llvm {
namespace trace {

const std::string TracePass::PassName = "dg-trace";

::llvm::PreservedAnalyses TracePass::run(::llvm::Module &M,
                                         ::llvm::ModuleAnalysisManager &AM) {
  // TODO: Implement trace functionality
  // This is a stub implementation for the generic trace pass
  LLVM_DEBUG(::llvm::dbgs() << "Running TracePass on module: " << M.getName()
                            << "\n");

  return ::llvm::PreservedAnalyses::all();
}

} // namespace trace
} // namespace llvm
} // namespace dragongem

#undef DEBUG_TYPE
