#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include <string>

namespace dragongem {
namespace llvm {
namespace trace {

class TracePass : public ::llvm::PassInfoMixin<TracePass> {
public:
  static const std::string PassName;

  ::llvm::PreservedAnalyses run(::llvm::Module &M,
                                ::llvm::ModuleAnalysisManager &AM);
};

} // namespace trace
} // namespace llvm
} // namespace dragongem
