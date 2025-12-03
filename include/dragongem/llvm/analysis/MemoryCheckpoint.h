#ifndef DRAGONGEM_LLVM_ANALYSIS_MEMORY_CHECKPOINT_H
#define DRAGONGEM_LLVM_ANALYSIS_MEMORY_CHECKPOINT_H

#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "dragongem/llvm/analysis/MemoryAlias.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/PassManager.h"

namespace dragongem {
namespace llvm {

struct ProgramRegion {
  const std::unordered_set<const ::llvm::BasicBlock *> BBs;
  const int NumIterations = 1;

  bool contains(const ::llvm::BasicBlock *const BB) const;
  bool contains(const ::llvm::BasicBlock &BB) const;
  bool contains(const ::llvm::Instruction *const Inst) const;
  bool contains(const ::llvm::Instruction &Inst) const;
};

class RegionNode {
public:
  enum RegionNodeKind {
    RNK_BB,
    RNK_Loop,
    RNK_Composite,
  };

private:
  RegionNodeKind Kind;

  RegionNode *parent_ = nullptr;
  std::vector<RegionNode *> children_;

public:
  RegionNode(const RegionNodeKind K);
  virtual ~RegionNode() = default;

  RegionNodeKind getKind() const;

  const std::vector<RegionNode *> &uses() const;
  const std::vector<RegionNode *> &users() const;

  RegionNode *parent() const;
  const std::vector<RegionNode *> &children() const;

  virtual std::vector<::llvm::BasicBlock *> &bbs() const;
  // Get the number of loads/stores present in this region.
  // For a loop, this would be <# allowed iters> * <# load/store>
  virtual int getNumLoads() const = 0;
  virtual int getNumStores() const = 0;

  // Is this node a candidate for merging.
  virtual bool isCandidate() const = 0;

  void addUse(RegionNode *const Use);
  void replaceAllUsesWith(RegionNode *const NewNode);
  void replaceAllUsesIf(RegionNode *const NewNode,
                        std::function<bool(RegionNode *const)> Predicate);
};

class RegionGraph {};

class MemoryCheckpointSolver {
public:
  using Checkpoints = std::vector<ProgramRegion>;

  MemoryCheckpointSolver(AliasInfo *const AI, ::llvm::LoopInfo *const LI,
                         ::llvm::ScalarEvolution *const SE, const int NumLoads,
                         const int NumStores, const bool EnforceMayAlias);

  Checkpoints solveRegions(const ::llvm::Function *const F) const;
  Checkpoints solveRegions(const ::llvm::Function &F) const;
  Checkpoints solveRegions(const ::llvm::Loop *const L) const;
  Checkpoints solveRegions(const ::llvm::Loop &L) const;

protected:
  AliasInfo *const AI;
  ::llvm::LoopInfo *const LI;
  ::llvm::ScalarEvolution *const SE;
  const int NumLoads;
  const int NumStores;
  const bool EnforceMayAlias;

private:
  mutable std::unordered_map<const ::llvm::Function *, Checkpoints>
      FunctionCache_;
  mutable std::unordered_map<const ::llvm::Loop *, Checkpoints> LoopCache_;
};

class MemoryCheckpointAnalysis
    : public ::llvm::AnalysisInfoMixin<MemoryCheckpointAnalysis> {
  friend ::llvm::AnalysisInfoMixin<MemoryCheckpointAnalysis>;

  static ::llvm::AnalysisKey Key;

public:
  using Result = MemoryCheckpointSolver;

  MemoryCheckpointAnalysis(const int NumLoads, const int NumStores,
                           const bool EnforceMayAlias);

  Result run(::llvm::Function &F, ::llvm::FunctionAnalysisManager &AM);
};

} // namespace llvm
} // namespace dragongem

#endif
