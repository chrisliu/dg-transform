#include "dragongem/llvm/Metadata.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"

#include <string>
#include <unordered_set>
#include <utility>

namespace dragongem {
namespace llvm {
namespace trace {

class MetadataPass : public ::llvm::PassInfoMixin<MetadataPass> {
private:
  using CallSet = std::unordered_set<const ::llvm::CallBase *>;

  static bool isTrackedFunction(const ::llvm::Function *const F);
  static bool isTrackedCall(const ::llvm::Instruction *const I);
  static bool isTrackedCall(const ::llvm::Instruction &I);

public:
  static const std::string PassName;

  ::llvm::PreservedAnalyses run(::llvm::Module &M,
                                ::llvm::ModuleAnalysisManager &AM);

private:
  std::pair<FunctionMetadataMap, CallSet>
  analyzeFunctions(::llvm::Module &M, ::llvm::ModuleAnalysisManager &MAM);

  LoopMetadataMap analyzeLoops(::llvm::Module &M,
                               ::llvm::ModuleAnalysisManager &MAM,
                               const FunctionMetadataMap &FMeta,
                               const CallSet &IndRecCalls);

  void analyzeLoop(const ::llvm::Loop *L, LoopMetadataMap &LMeta,
                   const ::llvm::LoopInfo &LI, const FunctionMetadataMap &FMeta,
                   const CallSet &IndRecCalls);

  void recordCall(const ::llvm::CallBase &Call, CallCount &ThisCallCount,
                  const FunctionMetadataMap &FMeta, const CallSet &IndRecCalls);

  void printStats(const FunctionMetadataMap &FMeta,
                  const LoopMetadataMap &LMeta);

  void serialize(::llvm::Module &M, ::llvm::ModuleAnalysisManager &MAM,
                 const FunctionMetadataMap &FMeta,
                 const LoopMetadataMap &LMeta);
};

} // namespace trace
} // namespace llvm
} // namespace dragongem
