#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/Support/Compiler.h>
#include <llvm/Support/Debug.h>

#define DEBUG_TYPE "dg-instrument-trace-pass"

namespace dragongem {

class InstrumentTracePass : public llvm::PassInfoMixin<InstrumentTracePass> {
public:
  llvm::PreservedAnalyses run(llvm::Function &f,
                              llvm::FunctionAnalysisManager &AM);
};

llvm::PreservedAnalyses
InstrumentTracePass::run(llvm::Function &F, llvm::FunctionAnalysisManager &AM) {
  LLVM_DEBUG(llvm::dbgs() << F.getName() << '\n');
  return llvm::PreservedAnalyses::all();
}

} // namespace dragongem

llvm::PassPluginLibraryInfo getInstrumentTracePassInfo() {
  return {LLVM_PLUGIN_API_VERSION, "DGInstrumentTracePass", "v0.1",
          [](llvm::PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](llvm::StringRef Name, llvm::FunctionPassManager &FPM,
                   llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) {
                  if (Name == "dg-instrument-trace-pass") {
                    FPM.addPass(dragongem::InstrumentTracePass());
                    return true;
                  }
                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getInstrumentTracePassInfo();
}

#undef DEBUG_TYPE
