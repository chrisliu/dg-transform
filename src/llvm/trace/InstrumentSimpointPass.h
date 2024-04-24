#include "InstrumentationInterface.h"
#include "dragongem/llvm/CanonicalId.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include <string>

namespace dragongem {
namespace llvm {
namespace trace {

class InstrumentSimpointPass
    : public ::llvm::PassInfoMixin<InstrumentSimpointPass> {
public:
  static const std::string PassName;

  ::llvm::PreservedAnalyses run(::llvm::Module &M,
                                ::llvm::ModuleAnalysisManager &AM);

protected:
  void instrumentBasicBlock(::llvm::BasicBlock &BB,
                            const InstrumentationInterface &II,
                            const CanonicalId &CID);
};

} // namespace trace
} // namespace llvm
} // namespace dragongem
