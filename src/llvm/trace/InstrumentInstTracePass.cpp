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
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
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

      // Do this afterwards since theer's bug with getExecutableBasicBlock.
      // ::llvm::Instruction ptrs are invalidated after insertion.
      if (BB.isLandingPad()) {
        instrumentLandingPad(*BB.getLandingPadInst(), II, CID);
      }
    }
  }

  return ::llvm::PreservedAnalyses::none();
}

void InstrumentInstTracePass::instrumentBBEnter(
    ::llvm::Instruction &I, const InstrumentationInterface &II,
    const CanonicalId &CID) {

  ::llvm::IRBuilder<> Builder(&I);

  {
    BBId Id = CID.bbId(I.getParent());
    std::vector<::llvm::Value *> Args = {
        ::llvm::ConstantInt::get(II.I64Ty, Id, true)};
    Builder.CreateCall(II.RecordBasicBlockFunc, Args);
  }

  if (I.getParent()->isEntryBlock()) {
    std::vector<::llvm::Value *> Args;
    Builder.CreateCall(II.RecordFunctionEntryFunc, Args);
  }
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
    auto *Load = ::llvm::cast<::llvm::LoadInst>(&I);
    ::llvm::Value *Addr = Load->getOperand(0);
    std::vector<::llvm::Value *> Args = {Id, Addr};
    Builder.CreateCall(II.RecordLoadInstFunc, Args);
    break;
  }

  case ::llvm::Instruction::Store: {
    auto *Store = ::llvm::cast<::llvm::StoreInst>(&I);
    ::llvm::Value *Addr = Store->getOperand(1);
    std::vector<::llvm::Value *> Args = {Id, Addr};
    Builder.CreateCall(II.RecordStoreInstFunc, Args);
    break;
  }

  case ::llvm::Instruction::Ret:
    Builder.CreateCall(II.RecordReturnInstFunc);
    break;

  case ::llvm::Instruction::CatchPad:
  case ::llvm::Instruction::CleanupPad:
    ::llvm::errs() << "Unsupported instruction: " << I << '\n';
    assert(false && "Unsupported instruction");
  }

  // Increment dynamic instruction count.
  Builder.CreateCall(II.IncDynamicInstCountFunc);
}

void InstrumentInstTracePass::instrumentLandingPad(
    ::llvm::LandingPadInst &I, const InstrumentationInterface &II,
    const CanonicalId &CID) {
  // Insert after landing pad instruction to maintain LLVM IR semantics.
  ::llvm::Instruction *NextInst = I.getNextNode();
  assert(NextInst);
  ::llvm::IRBuilder<> Builder(NextInst);

  const ::llvm::Function *F = I.getFunction();
  const BBId FId = CID.bbId(F->getEntryBlock());
  std::vector<::llvm::Value *> Args = {
      ::llvm::ConstantInt::get(II.I64Ty, FId, true),
  };
  Builder.CreateCall(II.RecordLandingPadFunc, Args);
}

} // namespace trace
} // namespace llvm
} // namespace dragongem

#undef DEBUG_TYPE
