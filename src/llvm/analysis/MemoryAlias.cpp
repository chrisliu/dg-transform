#include "dragongem/llvm/analysis/MemoryAlias.h"

#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/AliasSetTracker.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"

namespace dragongem {
namespace llvm {

MemoryAliasAnalysis::Result
MemoryAliasAnalysis::run(::llvm::Function &F,
                         ::llvm::FunctionAnalysisManager &AM) {

  ::llvm::MemorySSA &MSSA =
      AM.getResult<::llvm::MemorySSAAnalysis>(F).getMSSA();


  return Result{
      .MayAlias = identifyAlias(F, MSSA, true),
      .MustAlias = identifyAlias(F, MSSA, false),
  };
}

MemoryAliasAnalysis::Result::AAEdges
MemoryAliasAnalysis::identifyAlias(const ::llvm::Function &F,
                                   ::llvm::MemorySSA &MSSA,
                                   const bool OnlyMayAlias) const {
  ::llvm::AliasAnalysis &AA = MSSA.getAA();
  const auto IsClobber =
      [&AA, OnlyMayAlias](const ::llvm::MemoryUseOrDef *UseOrDef1,
                          const ::llvm::MemoryUseOrDef *UseOrDef2) -> bool {
    const ::llvm::AliasResult AR =
        AA.alias(UseOrDef1->getMemoryInst(), UseOrDef2->getMemoryInst());
    const bool IsMayAlias = AR == ::llvm::AliasResult::MayAlias;
    if (OnlyMayAlias) {
      return IsMayAlias;
    }
    return AR && !IsMayAlias; // Contextually convert NoAlias to false.
  };

  const auto IdentifyClobbers = [&IsClobber](
                                    const ::llvm::MemoryUseOrDef *Start) {
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
      if (::llvm::isa<::llvm::MemoryPhi>(MA)) {
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
  Result::AAEdges Producers;
  for (const auto &BB : F) {
    for (const auto &MA : *MSSA.getBlockAccesses(&BB)) {
      if (const auto *UseOrDef =
              ::llvm::dyn_cast<::llvm::MemoryUseOrDef>(&MA)) {
        for (const ::llvm::MemoryUseOrDef *Prod : IdentifyClobbers(UseOrDef)) {
          Producers[UseOrDef->getMemoryInst()].insert(Prod->getMemoryInst());
        }
      }
    }
  }
  return Producers;
}

} // namespace llvm
} // namespace dragongem
