#include "InstrumentInstTracePass.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iterator>
#include <vector>

#include "CLOpts.h"
#include "Instrumentation.h"
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
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#define DEBUG_TYPE "dg-instrument-inst-trace-pass"

namespace {
// TODO: This should be rolled into an executable basic block/binary class.
std::uint64_t getInstIdx(const ::llvm::Instruction &I,
                         const dragongem::llvm::ExecutableBasicBlock &XBB) {
  assert(!XBB.empty());
  const auto It = std::find(XBB.begin(), XBB.end(), &I);
  if (It == XBB.end()) {
    llvm::errs() << I << '\n';
    llvm::errs() << *I.getParent() << '\n';
  }
  assert(It != XBB.end());
  return static_cast<std::uint64_t>(std::distance(XBB.begin(), It));
}
} // namespace

namespace dragongem {
namespace llvm {
namespace trace {

const std::string InstrumentInstTracePass::PassName =
    "dg-instrument-inst-trace-pass";

bool InstrumentInstTracePass::isInstrumentedFunction(
    const ::llvm::Function *const F) {
  // We ignore intrinsic functions.
  // We have to conservatively instrument any indirect function calls.
  return !F || !F->isIntrinsic();
}

::llvm::PreservedAnalyses
InstrumentInstTracePass::run(::llvm::Module &M,
                             ::llvm::ModuleAnalysisManager &AM) {

  assert(!UIDFile.empty() && "Must provide the LLVM UID file");

  InstrumentationInterface II(M);
  CanonicalId CID(M, UIDFile);
  XBBMap XBB;

  for (::llvm::Function &F : M) {
    FunctionMetadata FMeta;
    for (::llvm::BasicBlock &BB : F) {
      if (!XBB.count(&BB)) {
        XBB[&BB] = getExecutableBasicBlock(BB);
      }

      LLVM_DEBUG(::llvm::dbgs() << "BB " << BB.getName() << '\n';
                 for (::llvm::Instruction *I
                      : XBB.at(&BB)) { ::llvm::dbgs() << *I << '\n'; });

      instrumentBB(BB, XBB, FMeta, II, CID);
    }

    for (::llvm::BasicBlock &BB : F) {
      for (::llvm::Instruction *const I : XBB.at(&BB)) {
        switch (I->getOpcode()) {
        case ::llvm::Instruction::Call: {
          auto *const Call = ::llvm::cast<::llvm::CallInst>(I);
          if (!isInstrumentedFunction(Call->getCalledFunction())) {
            break;
          }
          [[fallthrough]];
        }
        case ::llvm::Instruction::Invoke:
          assert(FMeta.CSHandle); // Should exist since we handle restore in
                                  // the first phase.
          instrumentGetCSHandle(*I, FMeta, II, CID);
          break;
        }

        ::llvm::IRBuilder<> Builder(I);
        Builder.CreateCall(II.IncDynamicInstCountFunc);
      }
    }
  }

  return ::llvm::PreservedAnalyses::none();
}

void InstrumentInstTracePass::instrumentBB(::llvm::BasicBlock &BB, XBBMap &XBB,
                                           FunctionMetadata &FMeta,
                                           const InstrumentationInterface &II,
                                           const CanonicalId &CID) {

  bool IsFirst = true;
  for (::llvm::Instruction *const I : XBB.at(&BB)) {
    LLVM_DEBUG(::llvm::dbgs() << "Instrumenting " << *I << '\n');

    if (IsFirst) {
      instrumentBBEnter(*I, II, CID);
      IsFirst = false;
    }

    instrumentInstruction(*I, XBB, FMeta, II, CID);
  }
}

void InstrumentInstTracePass::instrumentBBEnter(
    ::llvm::Instruction &I, const InstrumentationInterface &II,
    const CanonicalId &CID) {

  ::llvm::IRBuilder<> Builder(&I);

  BBId Id = CID.bbId(I.getParent());
  std::vector<::llvm::Value *> Args = {
      ::llvm::ConstantInt::get(II.I64Ty, Id, false),
      I.getParent()->isEntryBlock() ? II.TrueVal : II.FalseVal,
  };
  Builder.CreateCall(II.RecordBasicBlockFunc, Args);
}

void InstrumentInstTracePass::instrumentInstruction(
    ::llvm::Instruction &I, XBBMap &XBB, FunctionMetadata &FMeta,
    const InstrumentationInterface &II, const CanonicalId &CID) {

  ::llvm::ConstantInt *InstId =
      ::llvm::ConstantInt::get(II.I64Ty, CID.instId(I), false);
  ::llvm::IRBuilder<> Builder(&I);

  // Handle any instruction-op-specific instrumentation.
  switch (I.getOpcode()) {

  case ::llvm::Instruction::Load: {
    auto *Load = ::llvm::cast<::llvm::LoadInst>(&I);
    ::llvm::Value *Addr = Load->getOperand(0);
    std::vector<::llvm::Value *> Args = {InstId, Addr};
    Builder.CreateCall(II.RecordLoadInstFunc, Args);
    break;
  }

  case ::llvm::Instruction::Store: {
    auto *Store = ::llvm::cast<::llvm::StoreInst>(&I);
    ::llvm::Value *Addr = Store->getOperand(1);
    std::vector<::llvm::Value *> Args = {InstId, Addr};
    Builder.CreateCall(II.RecordStoreInstFunc, Args);
    break;
  }

  case ::llvm::Instruction::Call: {
    auto *const Call = ::llvm::cast<::llvm::CallInst>(&I);

    if (!isInstrumentedFunction(Call->getCalledFunction())) {
      assert(Call->getCalledFunction());
      ::llvm::dbgs() << "Ignored " << Call->getCalledFunction()->getName()
                     << '\t' << *Call << '\n';
      break;
    }

    // if (::llvm::Function *const CalledF = Call->getCalledFunction()) {
    //   LLVM_DEBUG(::llvm::dbgs() << CalledF->isDeclaration() << '\t'
    //                             << CalledF->getName() << '\t' << *Call <<
    //                             '\n');
    //   if (CalledF->isDeclaration()) {
    //     ::llvm::dbgs() << "Ignored " << CalledF->getName() << '\t' << *Call
    //                    << '\n';
    //     break;
    //   }
    // }

    if (!FMeta.CSHandle) {
      initCSHandle(*I.getFunction(), FMeta, II);
    }
    instrumentCallInstruction(I, XBB.at(I.getParent()), FMeta, II, CID);
    break;
  }

  case ::llvm::Instruction::Invoke: {
    if (!FMeta.CSHandle) {
      initCSHandle(*I.getFunction(), FMeta, II);
    }
    instrumentInvokeInstruction(I, XBB, FMeta, II, CID);
    break;
  }

  case ::llvm::Instruction::CallBr:
  case ::llvm::Instruction::CatchSwitch:
  case ::llvm::Instruction::CatchRet:
  case ::llvm::Instruction::CatchPad:
  case ::llvm::Instruction::CleanupPad:
  case ::llvm::Instruction::CleanupRet:
    ::llvm::errs() << "Unsupported instruction: " << I << '\n';
    assert(false && "Unsupported instruction");
  }

  /*// Increment dynamic instruction count.*/
  /*Builder.CreateCall(II.IncDynamicInstCountFunc);*/
}

void InstrumentInstTracePass::instrumentCallInstruction(
    ::llvm::Instruction &I, const ExecutableBasicBlock &XBB,
    FunctionMetadata &FMeta, const InstrumentationInterface &II,
    const CanonicalId &CID) {

  // instrumentGetCSHandle(I, FMeta, II, CID);

  const std::uint64_t Idx = getInstIdx(I, XBB);
  ::llvm::Instruction *const NextI = XBB.at(Idx + 1);
  instrumentRestoreCSHandle(*NextI, XBB, FMeta, II);
}

void InstrumentInstTracePass::instrumentInvokeInstruction(
    ::llvm::Instruction &I, XBBMap &XBB, FunctionMetadata &FMeta,
    const InstrumentationInterface &II, const CanonicalId &CID) {

  // instrumentGetCSHandle(I, FMeta, II, CID);

  auto *const Invoke = ::llvm::cast<::llvm::InvokeInst>(&I);
  {
    ::llvm::BasicBlock *NBB = Invoke->getNormalDest();
    if (!XBB.count(NBB)) {
      XBB[NBB] = getExecutableBasicBlock(*NBB);
    }
    ::llvm::Instruction *NI = XBB.at(NBB).front();
    instrumentRestoreCSHandle(*NI, XBB.at(NBB), FMeta, II);
  }
  {
    ::llvm::BasicBlock *UBB = Invoke->getUnwindDest();
    if (!XBB.count(UBB)) {
      XBB[UBB] = getExecutableBasicBlock(*UBB);
    }
    ::llvm::Instruction *UI = XBB.at(UBB).front();
    instrumentRestoreCSHandle(*UI, XBB.at(UBB), FMeta, II);
  }
}

void InstrumentInstTracePass::initCSHandle(::llvm::Function &F,
                                           FunctionMetadata &FMeta,
                                           const InstrumentationInterface &II) {
  // Create variable.
  {
    ::llvm::IRBuilder<> Builder(&*F.getEntryBlock().begin());
    FMeta.CSHandle = Builder.CreateAlloca(II.I64Ty);
  }

  // Assign to invalid.
  {
    ::llvm::IRBuilder<> Builder(
        &*F.getEntryBlock().getFirstNonPHIOrDbgOrAlloca());
    Builder.CreateStore(::llvm::ConstantInt::get(II.I64Ty, InvalidCall, false),
                        FMeta.CSHandle);
  }
}

void InstrumentInstTracePass::instrumentGetCSHandle(
    ::llvm::Instruction &I, const FunctionMetadata &FMeta,
    const InstrumentationInterface &II, const CanonicalId &CID) {

  assert(FMeta.CSHandle);
  ::llvm::IRBuilder<> Builder(&I);
  std::vector<::llvm::Value *> Args = {
      ::llvm::ConstantInt::get(II.I64Ty, CID.instId(I), false),
  };
  auto *const CSHandleVal = Builder.CreateCall(II.GetCallSiteFunc, Args);
  Builder.CreateStore(CSHandleVal, FMeta.CSHandle);
}

void InstrumentInstTracePass::instrumentRestoreCSHandle(
    ::llvm::Instruction &I, const ExecutableBasicBlock &XBB,
    FunctionMetadata &FMeta, const InstrumentationInterface &II) {

  assert(FMeta.CSHandle);

  if (FMeta.CSRestoreInsts.count(&I)) {
    return;
  }
  FMeta.CSRestoreInsts.insert(&I);

  ::llvm::IRBuilder<> Builder(&I);
  auto *const CSHandleVal = Builder.CreateLoad(II.I64Ty, FMeta.CSHandle);

  const std::uint64_t NumRetired = getInstIdx(I, XBB);

  std::vector<::llvm::Value *> CallArgs = {
      CSHandleVal,
      ::llvm::ConstantInt::get(II.I64Ty, NumRetired, false),
  };
  Builder.CreateCall(II.RecordReturnFromCallFunc, CallArgs);
  Builder.CreateStore(::llvm::ConstantInt::get(II.I64Ty, InvalidCall, false),
                      FMeta.CSHandle);
}

} // namespace trace
} // namespace llvm
} // namespace dragongem

#undef DEBUG_TYPE
