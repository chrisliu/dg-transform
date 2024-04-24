#include "InstrumentInstTracePass.h"
#include "CLOpts.h"
#include "InstrumentationInterface.h"
#include "dragongem/llvm/CanonicalId.h"
#include "dragongem/llvm/ExecutableBasicBlock.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/Casting.h"
#include <cassert>
#include <vector>

#define DEBUG_TYPE "dg-instrument-inst-trace-pass"

namespace dragongem {
namespace llvm {
namespace trace {

const std::string InstrumentInstTracePass::PassName =
    "dg-instrument-inst-trace-pass";

::llvm::PreservedAnalyses
InstrumentInstTracePass::run(::llvm::Module &M,
                             ::llvm::ModuleAnalysisManager &AM) {
  assert(!UIDFile.empty() && "Must provide the LLVM UID file");

  InstrumentationInterface II(M);
  CanonicalId CID(M, UIDFile);

  for (::llvm::Function &F : M) {
    for (::llvm::BasicBlock &BB : F) {
      bool IsFirst = true;
      for (::llvm::Instruction *I : getExecutableBasicBlock(BB)) {
        if (IsFirst) {
          instrumentBBEnter(*I, II, CID);
          IsFirst = false;
        }
        instrumentInstruction(*I, II, CID);
      }
    }
  }

  return ::llvm::PreservedAnalyses::none();
}

void InstrumentInstTracePass::instrumentBBEnter(
    ::llvm::Instruction &I, const InstrumentationInterface &II,
    const CanonicalId &CID) {

  BBId Id = CID.bbId(I.getParent());
  ::llvm::IRBuilder<> Builder(&I);
  std::vector<::llvm::Value *> Args = {
      ::llvm::ConstantInt::get(II.I64Ty, Id, true)};
  Builder.CreateCall(II.RecordBasicBlockFunc, Args);
}

void InstrumentInstTracePass::instrumentInstruction(
    ::llvm::Instruction &I, const InstrumentationInterface &II,
    const CanonicalId &CID) {

  ::llvm::ConstantInt *Id =
      ::llvm::ConstantInt::get(II.I64Ty, CID.instId(I), true);
  ::llvm::IRBuilder<> Builder(&I);

  // Handle any instruction class-specific instrumentation.
  switch (I.getOpcode()) {
  case ::llvm::Instruction::Load: {
    auto *Load = ::llvm::dyn_cast<::llvm::LoadInst>(&I);
    ::llvm::Value *Addr = Load->getOperand(0);
    std::vector<::llvm::Value *> Args = {Id, Addr};
    Builder.CreateCall(II.RecordLoadInstFunc, Args);
    break;
  }
  case ::llvm::Instruction::Store: {
    auto *Store = ::llvm::dyn_cast<::llvm::StoreInst>(&I);
    ::llvm::Value *Addr = Store->getOperand(1);
    std::vector<::llvm::Value *> Args = {Id, Addr};
    Builder.CreateCall(II.RecordStoreInstFunc, Args);
    break;
  }
  }

  // Increment dynamic instruction count.
  Builder.CreateCall(II.IncDynamicInstCountFunc);
}

} // namespace trace
} // namespace llvm
} // namespace dragongem

#undef DEBUG_TYPE
