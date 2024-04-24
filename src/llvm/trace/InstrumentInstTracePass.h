#include "InstrumentationInterface.h"
#include "dragongem/llvm/CanonicalId.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"

namespace dragongem {
namespace llvm {
namespace trace {

class InstrumentInstTracePass
    : public ::llvm::PassInfoMixin<InstrumentInstTracePass> {
public:
  static const std::string PassName;

  ::llvm::PreservedAnalyses run(::llvm::Module &M,
                                ::llvm::ModuleAnalysisManager &AM);

protected:
  void instrumentBBEnter(::llvm::Instruction &I,
                         const InstrumentationInterface &II,
                         const CanonicalId &CID);

  void instrumentInstruction(::llvm::Instruction &I,
                             const InstrumentationInterface &II,
                             const CanonicalId &CID);
};

} // namespace trace
} // namespace llvm
} // namespace dragongem
