#ifndef DRAGONGEM_LLVM_ANALYSIS_MANAGER_H
#define DRAGONGEM_LLVM_ANALYSIS_MANAGER_H

#include "llvm/Analysis/CGSCCPassManager.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/IR/PassManager.h"

namespace dragongem {
namespace llvm {

class LLVMAnalysisManager {
public:
  LLVMAnalysisManager();

  ::llvm::LoopAnalysisManager &getLAM();
  ::llvm::FunctionAnalysisManager &getFAM();
  ::llvm::CGSCCAnalysisManager &getCGAM();
  ::llvm::ModuleAnalysisManager &getMAM();

private:
  // Create analysis managers.
  // NOTE: Please don't reorder analysis manager initialization. These are
  //       declared such that they'll be destroyed in the correct order due
  //       to inter-analysis-manager references.
  ::llvm::LoopAnalysisManager LoopManager_;
  ::llvm::FunctionAnalysisManager FunctionManager_;
  ::llvm::CGSCCAnalysisManager CGSCCManager_;
  ::llvm::ModuleAnalysisManager ModuleManager_;
};

} // namespace llvm
} // namespace dragongem

#endif // DRAGONGEM_LLVM_ANALYSIS_MANAGER_H
