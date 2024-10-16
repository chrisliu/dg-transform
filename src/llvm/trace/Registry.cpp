#include "InstrumentInstTracePass.h"
#include "InstrumentSimpointPass.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Transforms/Utils/Mem2Reg.h"

// TODO: Use a registry class.
llvm::PassPluginLibraryInfo getInstrumentPassesPluginInfo() {
  return {
      LLVM_PLUGIN_API_VERSION, "DGInstrumentPasses", "v0.1",
      [](llvm::PassBuilder &PB) {
        PB.registerPipelineParsingCallback(
            [](llvm::StringRef Name, llvm::ModulePassManager &MPM,
               llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) {
              if (Name ==
                  dragongem::llvm::trace::InstrumentSimpointPass::PassName) {
                MPM.addPass(dragongem::llvm::trace::InstrumentSimpointPass());
                return true;
              } else if (Name == dragongem::llvm::trace::
                                     InstrumentInstTracePass::PassName) {
                MPM.addPass(dragongem::llvm::trace::InstrumentInstTracePass());
                // Run mem2reg for alloca insts inserted at call sites.
                MPM.addPass(::llvm::createModuleToFunctionPassAdaptor(
                    ::llvm::PromotePass()));
                return true;
              }
              return false;
            });
      }};
}

extern "C" LLVM_ATTRIBUTE_WEAK llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getInstrumentPassesPluginInfo();
}
