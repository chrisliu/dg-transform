#include "dragongem/llvm/ExecutableBasicBlock.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Casting.h"

namespace dragongem {
namespace llvm {

ExecutableBasicBlock getExecutableBasicBlock(::llvm::BasicBlock &BB) {
  ExecutableBasicBlock ExecBB;

  for (auto It = BB.getFirstNonPHIOrDbgOrLifetime()->getIterator();
       It != BB.end(); ++It) {
    if (isExecInst(*It)) {
      ExecBB.push_back(&*It);
    }
  }

  assert(!ExecBB.empty());
  return ExecBB;
}

ExecutableConstBasicBlock
getExecutableBasicBlock(const ::llvm::BasicBlock &BB) {
  ExecutableConstBasicBlock ExecBB;

  for (auto It = BB.getFirstNonPHIOrDbgOrLifetime()->getIterator();
       It != BB.end(); ++It) {
    if (isExecInst(*It)) {
      ExecBB.push_back(&*It);
    }
  }

  assert(!ExecBB.empty());
  return ExecBB;
}

bool isExecInst(const ::llvm::Instruction &I) {
  return !(I.isDebugOrPseudoInst() || I.isLifetimeStartOrEnd() ||
           ::llvm::isa<::llvm::LandingPadInst>(I));
}
} // namespace llvm
} // namespace dragongem
