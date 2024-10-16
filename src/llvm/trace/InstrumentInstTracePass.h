#include <unordered_map>
#include <unordered_set>

#include "InstrumentationInterface.h"
#include "dragongem/llvm/CanonicalId.h"
#include "dragongem/llvm/ExecutableBasicBlock.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"

namespace dragongem {
namespace llvm {
namespace trace {

class InstrumentInstTracePass
    : public ::llvm::PassInfoMixin<InstrumentInstTracePass> {
protected:
  struct FunctionMetadata {
    ::llvm::AllocaInst *CSHandle = nullptr;
    std::unordered_set<::llvm::Instruction *> CSRestoreInsts;
  };

  using XBBMap = std::unordered_map<::llvm::BasicBlock *, ExecutableBasicBlock>;

  static bool isInstrumentedFunction(const ::llvm::Function *const F);

public:
  static const std::string PassName;

  ::llvm::PreservedAnalyses run(::llvm::Module &M,
                                ::llvm::ModuleAnalysisManager &AM);

protected:
  void instrumentBB(::llvm::BasicBlock &BB, XBBMap &XBB,
                    FunctionMetadata &FMeta, const InstrumentationInterface &II,
                    const CanonicalId &CID);
  void instrumentBBEnter(::llvm::Instruction &FirstInst,
                         const InstrumentationInterface &II,
                         const CanonicalId &CID);
  void instrumentInstruction(::llvm::Instruction &I, XBBMap &XBB,
                             FunctionMetadata &FMeta,
                             const InstrumentationInterface &II,
                             const CanonicalId &CID);
  void instrumentCallInstruction(::llvm::Instruction &I,
                                 const ExecutableBasicBlock &XBB,
                                 FunctionMetadata &FMeta,
                                 const InstrumentationInterface &II,
                                 const CanonicalId &CID);
  void instrumentInvokeInstruction(::llvm::Instruction &I, XBBMap &XBB,
                                   FunctionMetadata &FMeta,
                                   const InstrumentationInterface &II,
                                   const CanonicalId &CID);

  // Create function local variable for the call site handle.
  void initCSHandle(::llvm::Function &F, FunctionMetadata &FMeta,
                    const InstrumentationInterface &II);
  // Insert before I.
  void instrumentGetCSHandle(::llvm::Instruction &I,
                             const FunctionMetadata &FMeta,
                             const InstrumentationInterface &II,
                             const CanonicalId &CID);
  // Insert before I.
  void instrumentRestoreCSHandle(::llvm::Instruction &I,
                                 const ExecutableBasicBlock &XBB,
                                 FunctionMetadata &FMeta,
                                 const InstrumentationInterface &II);
};

} // namespace trace
} // namespace llvm
} // namespace dragongem
