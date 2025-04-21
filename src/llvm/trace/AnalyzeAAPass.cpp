#include "AnalyzeAAPass.h"

#include "llvm/ADT/SetVector.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Support/Casting.h"

#include <cassert>
#include <unordered_set>

namespace dragongem {
namespace llvm {

::llvm::PreservedAnalyses
AnalyzeAAPass::run(::llvm::Module &M, ::llvm::ModuleAnalysisManager &AM) {

  ::llvm::FunctionAnalysisManager &FAM =
      AM.getResult<::llvm::FunctionAnalysisManagerModuleProxy>(M).getManager();

  for (::llvm::Function &F : M) {
    auto &MSSA = FAM.getResult<::llvm::MemorySSAAnalysis>(F).getMSSA();
    analyzeFunction(F, MSSA, ::llvm::AliasResult::MustAlias);
  }

  return ::llvm::PreservedAnalyses::none();
}

void AnalyzeAAPass::analyzeFunction(::llvm::Function &F,
                                    ::llvm::MemorySSA &MSSA,
                                    const bool EnforceMayAlias) {

  ::llvm::AliasAnalysis &AA = MSSA.getAA();
  const auto IsClobber =
      [&AA, EnforceMayAlias](const ::llvm::MemoryUseOrDef *UseOrDef1,
                             const ::llvm::MemoryUseOrDef *UseOrDef2) -> bool {
    const ::llvm::AliasResult AR =
        AA.alias(UseOrDef1->getMemoryInst(), UseOrDef2->getMemoryInst());
    if (AR == ::llvm::AliasResult::MayAlias) {
      return EnforceMayAlias;
    }
    return AR; // Contextually convert NoAlias to false.
  };

  const auto IdentifyClobbers = [&IsClobber,
                                 &MSSA](const ::llvm::MemoryUseOrDef *Start) {
    std::unordered_set<const ::llvm::MemoryUseOrDef *> Clobbers;
    std::unordered_set<const ::llvm::MemoryAccess *> Visited;
    std::vector<const ::llvm::MemoryAccess *> WorkList;

    for (const auto *User : Start->users()) {
      WorkList.push_back(::llvm::cast<::llvm::MemoryAccess>(User));
    }
    Visited.insert(Start);

    while (!WorkList.empty()) {
      const ::llvm::MemoryAccess *MA = WorkList.back();
      WorkList.pop_back();

      if (!Visited.insert(MA).second) {
        continue;
      }

      bool CheckUsers = false;
      if (const auto *Phi = ::llvm::dyn_cast<::llvm::MemoryPhi>(MA)) {
        CheckUsers = true;
      } else if (const auto *Use = ::llvm::dyn_cast<::llvm::MemoryUse>(MA)) {
        if (IsClobber(Start, Use)) {
          Clobbers.insert(Use);
        }
      } else {
        const auto *Def = ::llvm::cast<::llvm::MemoryDef>(MA);

        if (IsClobber(Start, Def)) {
          Clobbers.insert(Def);
        } else {
          CheckUsers = true;
        }
      }

      if (CheckUsers) {
        for (const auto *User : MA->users()) {
          WorkList.push_back(::llvm::cast<::llvm::MemoryAccess>(User));
        }
      }
    }

    return Clobbers;
  };

  // Identify clobbered instructions.
  for (const auto &BB : F) {
    for (const auto &MA : *MSSA.getBlockAccesses(&BB)) {
      if (const auto *UseOrDef =
              ::llvm::dyn_cast<::llvm::MemoryUseOrDef>(&MA)) {
        const auto Clobbers = IdentifyClobbers(UseOrDef);
      }
    }
  }
}

} // namespace llvm
} // namespace dragongem
