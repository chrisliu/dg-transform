#include "dragongem/llvm/AnalysisManager.h"
#include "llvm/Passes/PassBuilder.h"

namespace dragongem {
namespace llvm {

LLVMAnalysisManager::LLVMAnalysisManager() {
  ::llvm::PassBuilder pass_builder;
  pass_builder.registerModuleAnalyses(ModuleManager_);
  pass_builder.registerCGSCCAnalyses(CGSCCManager_);
  pass_builder.registerFunctionAnalyses(FunctionManager_);
  pass_builder.registerLoopAnalyses(LoopManager_);
  pass_builder.crossRegisterProxies(LoopManager_, FunctionManager_,
                                    CGSCCManager_, ModuleManager_);
}

::llvm::LoopAnalysisManager &LLVMAnalysisManager::getLAM() {
  return LoopManager_;
}

::llvm::FunctionAnalysisManager &LLVMAnalysisManager::getFAM() {
  return FunctionManager_;
}

::llvm::CGSCCAnalysisManager &LLVMAnalysisManager::getCGAM() {
  return CGSCCManager_;
}

::llvm::ModuleAnalysisManager &LLVMAnalysisManager::getMAM() {
  return ModuleManager_;
}

} // namespace llvm
} // namespace dragongem
