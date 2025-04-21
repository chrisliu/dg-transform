#ifndef DRAGONGEM_LLVM_CANONICAL_ID_H
#define DRAGONGEM_LLVM_CANONICAL_ID_H

#include <cstdint>
#include <filesystem>
#include <optional>
#include <unordered_map>
#include <vector>

#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"

namespace dragongem {
namespace llvm {

using InstId = std::uint64_t;
using BBId = std::uint64_t;
using LoopId = std::uint64_t;
using FunctionId = BBId;

class CanonicalId {
public:
  static constexpr InstId InvalidInstId = 0;
  static constexpr InstId FirstInstId = 1;
  static constexpr BBId InvalidBBId = 0;
  static constexpr BBId FirstBBId = 1;
  static constexpr LoopId InvalidLoopId = 0;
  static constexpr LoopId FirstLoopId = 0;

  CanonicalId(const ::llvm::Module &M, ::llvm::FunctionAnalysisManager &FAM);
  CanonicalId(const ::llvm::Module *const M,
              ::llvm::FunctionAnalysisManager &FAM);
  CanonicalId(const ::llvm::Module &M, ::llvm::FunctionAnalysisManager &FAM,
              const std::filesystem::path UIDFile);
  CanonicalId(const ::llvm::Module *const M,
              ::llvm::FunctionAnalysisManager &FAM,
              const std::filesystem::path UIDFile);

  InstId instId(const ::llvm::Instruction &I) const;
  InstId instId(const ::llvm::Instruction *const I) const;

  BBId bbId(const ::llvm::BasicBlock &BB) const;
  BBId bbId(const ::llvm::BasicBlock *const BB) const;

  LoopId loopId(const ::llvm::Loop &L) const;
  LoopId loopId(const ::llvm::Loop *const L) const;

  FunctionId functionId(const ::llvm::Function &F) const;
  FunctionId functionId(const ::llvm::Function *const F) const;

  const ::llvm::Instruction *getInst(InstId Id) const;
  const ::llvm::BasicBlock *getBB(BBId Id) const;
  const ::llvm::Loop *getLoop(LoopId Id) const;
  const ::llvm::Function *getFunction(FunctionId Id) const;

  bool hasInst(InstId Id) const;
  bool hasBB(BBId Id) const;
  bool hasLoop(LoopId Id) const;
  bool hasFunction(FunctionId Id) const;

  std::uint64_t numInsts() const;
  std::uint64_t numBBs() const;
  std::uint64_t numLoops() const;

  void serialize(std::filesystem::path UIDFile) const;

protected:
  std::unordered_map<const ::llvm::Instruction *, InstId> InstToId;
  std::unordered_map<const ::llvm::BasicBlock *, BBId> BBToId;
  std::unordered_map<const ::llvm::Loop *, LoopId> LoopToId;

  std::unordered_map<InstId, const ::llvm::Instruction *> IdToInst;
  std::unordered_map<BBId, const ::llvm::BasicBlock *> IdToBB;
  std::unordered_map<LoopId, const ::llvm::Loop *> IdToLoop;

  void buildReverseMaps();

public:
  static std::string getFuncName(const ::llvm::Function &F);
  static std::string getFuncName(const ::llvm::Function *const F);

  static std::string getBBName(const ::llvm::BasicBlock &BB);
  static std::string getBBName(const ::llvm::BasicBlock *const BB);

protected:
  // Serialization metadata.
  struct BBMetadata {
    const ::llvm::BasicBlock *BB;
    BBId Id;
    InstId InstStartId;
    std::optional<LoopId> ThisLoopId;
  };

  std::vector<BBMetadata> BBMeta;
};

} // namespace llvm
} // namespace dragongem

#endif // DRAGONGEM_LLVM_CANONICAL_ID_H
