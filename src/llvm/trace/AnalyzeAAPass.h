#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"

#include <string>

namespace dragongem {
namespace llvm {

class AnalyzeAAPass : public ::llvm::PassInfoMixin<AnalyzeAAPass> {
public:
  static const std::string PassName;

  ::llvm::PreservedAnalyses run(::llvm::Module &M,
                                ::llvm::ModuleAnalysisManager &AM);

protected:
  void analyzeFunction(::llvm::Function &F, ::llvm::MemorySSA &MSSA,
                       const bool EnforceMayAlias);
};

} // namespace llvm
} // namespace dragongem
