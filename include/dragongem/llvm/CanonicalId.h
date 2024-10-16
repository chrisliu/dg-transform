#ifndef DRAGONGEM_LLVM_CANONICAL_ID_H
#define DRAGONGEM_LLVM_CANONICAL_ID_H

#include <cstdint>
#include <filesystem>
#include <unordered_map>
#include <vector>

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"

namespace dragongem {
namespace llvm {

using InstId = std::uint64_t;
using BBId = std::uint64_t;

class CanonicalId {
public:
  static const InstId InvalidInstId = 0;
  static const InstId FirstInstId = 1;
  static const BBId InvalidBBId = 0;
  static const BBId FirstBBId = 1;

  explicit CanonicalId(const ::llvm::Module &M);
  explicit CanonicalId(const ::llvm::Module *const M);
  CanonicalId(const ::llvm::Module &M, std::filesystem::path UIDFile);
  CanonicalId(const ::llvm::Module *const M, std::filesystem::path UIDFile);

  InstId instId(const ::llvm::Instruction &I) const;
  InstId instId(const ::llvm::Instruction *const I) const;

  BBId bbId(const ::llvm::BasicBlock &BB) const;
  BBId bbId(const ::llvm::BasicBlock *const BB) const;

  const ::llvm::Instruction *getInst(InstId Id) const;
  const ::llvm::BasicBlock *getBB(BBId Id) const;

  bool hasInst(InstId Id) const;
  bool hasBB(BBId Id) const;

  std::uint64_t numInsts() const;
  std::uint64_t numBBs() const;

  void serialize(std::filesystem::path UIDFile) const;

protected:
  std::unordered_map<const ::llvm::Instruction *, InstId> InstToId;
  std::unordered_map<const ::llvm::BasicBlock *, BBId> BBToId;
  std::unordered_map<InstId, const ::llvm::Instruction *> IdToInst;
  std::unordered_map<BBId, const ::llvm::BasicBlock *> IdToBB;

  static std::string getFuncName(const ::llvm::Function &F);
  static std::string getFuncName(const ::llvm::Function *const F);

  static std::string getBBName(const ::llvm::BasicBlock &BB);
  static std::string getBBName(const ::llvm::BasicBlock *const BB);

  void buildReverseMaps();

protected:
  // Serialization metadata.
  struct BBMetadata {
    const ::llvm::BasicBlock *BB;
    BBId Id;
    InstId InstStartId;
  };

  std::vector<BBMetadata> BBMeta;
};

} // namespace llvm
} // namespace dragongem

#endif // DRAGONGEM_LLVM_CANONICAL_ID_H
