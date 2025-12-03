#ifndef DRAGONGEM_LLVM_ANALYSIS_MEMORY_ALIAS_H
#define DRAGONGEM_LLVM_ANALYSIS_MEMORY_ALIAS_H

#include <unordered_map>
#include <unordered_set>

#include "llvm/Analysis/MemorySSA.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/PassManager.h"

namespace dragongem {
namespace llvm {

struct AliasInfo {
  using AAEdges = std::unordered_map<::llvm::Instruction *,
                                     std::unordered_set<::llvm::Instruction *>>;

  const AAEdges MayAlias;
  const AAEdges MustAlias;
};

class MemoryAliasAnalysis
    : public ::llvm::AnalysisInfoMixin<MemoryAliasAnalysis> {
  friend ::llvm::AnalysisInfoMixin<MemoryAliasAnalysis>;

  static ::llvm::AnalysisKey Key;

public:
  using Result = AliasInfo;
  Result run(::llvm::Function &F, ::llvm::FunctionAnalysisManager &AM);

protected:
  Result::AAEdges identifyAlias(const ::llvm::Function &F,
                                ::llvm::MemorySSA &MSSA,
                                const bool OnlyMayAlias) const;
};

} // namespace llvm
} // namespace dragongem

#endif
