#include "InstrumentSimpointPass.h"
#include "CLOpts.h"
#include "InstrumentationInterface.h"
#include "dragongem/llvm/CanonicalId.h"
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Type.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/Support/Compiler.h>
#include <llvm/Support/Debug.h>

#define DEBUG_TYPE "dg-instrument-simpoint-pass"

namespace dragongem {
namespace llvm {
namespace trace {

const std::string InstrumentSimpointPass::PassName =
    "dg-instrument-simpoint-pass";

::llvm::PreservedAnalyses
InstrumentSimpointPass::run(::llvm::Module &M,
                            ::llvm::ModuleAnalysisManager &AM) {
  InstrumentationInterface II(M);
  CanonicalId CID(M);

  for (::llvm::Function &F : M) {
    for (::llvm::BasicBlock &BB : F) {
      LLVM_DEBUG(::llvm::dbgs() << CID.bbId(BB) << " " << F.getName() << "::");
      LLVM_DEBUG(BB.printAsOperand(::llvm::dbgs(), false);
                 ::llvm::dbgs() << "\n";);
      instrumentBasicBlock(BB, II, CID);
    }
  }

  LLVM_DEBUG(::llvm::dbgs() << UIDFile << '\n');

  if (UIDFile.empty()) {
    ::llvm::errs() << "Warning: UID file is not written\n";
  } else {
    CID.serialize(UIDFile);
  }

  // LLVM_DEBUG(M.dump());

  return ::llvm::PreservedAnalyses::none();
}

void InstrumentSimpointPass::instrumentBasicBlock(
    ::llvm::BasicBlock &BB, const InstrumentationInterface &II,
    const CanonicalId &CID) {
  for (auto InstIt = BB.getFirstNonPHIOrDbgOrAlloca(); InstIt != BB.end();
       InstIt++) {
    ::llvm::IRBuilder<> Builder(&*InstIt);

    // Add basic block vector instrumentation.
    if (InstIt == BB.getFirstNonPHIOrDbgOrAlloca()) {
      std::vector<::llvm::Value *> Args = {
          ::llvm::ConstantInt::get(II.I64Ty, CID.bbId(BB), false),
          BB.isEntryBlock() ? II.TrueVal : II.FalseVal,
      };
      Builder.CreateCall(II.RecordBasicBlockFunc, Args);
    }

    // Add instruction count instrumentation.
    Builder.CreateCall(II.IncDynamicInstCountFunc);
  }
  // LLVM_DEBUG(BB.print(::llvm::dbgs()));
}

} // namespace trace
} // namespace llvm
} // namespace dragongem

#undef DEBUG_TYPE
