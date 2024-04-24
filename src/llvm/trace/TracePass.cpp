#include <llvm/ADT/StringRef.h>
#include <llvm/Pass.h>

#define DEBUG_TYPE "dg-instrument-trace-pass"

namespace dragongem {

class InstrumentPass : public llvm::FunctionPass {
public:
  static char ID;

  bool doInitialization(llvm::Module &M) override;
  bool runOnFunction(llvm::Function &F) override;
};

bool InstrumentPass::doInitialization(llvm::Module &M) { return true; }

bool runOnFunction(llvm::Function &F) { return true; }

char InstrumentPass::ID = 0;
static llvm::RegisterPass<InstrumentPass> X("dg-instrument-trace-pass",
                                       "Instrumented trace pass", false, false);

} // namespace dragongem

#undef DEBUG_TYPE
