#include "dragongem/llvm/CanonicalId.h"
#include "stream.hpp"
#include "trace/LLVMUID.pb.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include <cassert>
#include <cstdint>
#include <fstream>
#include <functional>
#include <string>
#include <unordered_map>
#include <utility>

namespace dragongem {
namespace llvm {

CanonicalId::CanonicalId(const ::llvm::Module &M) {
  InstId CurInstId = FirstInstId;
  BBId CurBBId = FirstBBId;
  for (const ::llvm::Function &F : M) {
    for (const ::llvm::BasicBlock &BB : F) {
      BBMeta.emplace_back(
          BBMetadata{.BB = &BB, .Id = CurBBId, .InstStartId = CurInstId});
      BBToId[&BB] = CurBBId;
      CurBBId++;

      for (const ::llvm::Instruction &I : BB) {
        InstToId[&I] = CurInstId;
        CurInstId++;
      }
    }
  }
  buildReverseMaps();
}

CanonicalId::CanonicalId(const ::llvm::Module *const M) : CanonicalId(*M) {}

CanonicalId::CanonicalId(const ::llvm::Module &M,
                         std::filesystem::path UIDFile) {
  // Load BBMeta from UIDFile.
  using NameToBBMap =
      std::unordered_map<std::string, const ::llvm::BasicBlock *>;
  std::unordered_map<std::string, NameToBBMap> FuncNameToBBMap;
  for (const ::llvm::Function &F : M) {
    NameToBBMap FBB;
    for (const ::llvm::BasicBlock &BB : F) {
      FBB.emplace(getBBName(BB), &BB);
    }
    FuncNameToBBMap[getFuncName(F)] = FBB;
  }

  std::function<void(trace::CanonicalBB &)> ParseProtobuf =
      [&BBMeta = BBMeta, &FuncNameToBBMap = std::as_const(FuncNameToBBMap)](
          trace::CanonicalBB &CBB) {
        assert(FuncNameToBBMap.count(CBB.function_name()));
        const NameToBBMap &BBMap = FuncNameToBBMap.at(CBB.function_name());
        assert(BBMap.count(CBB.basic_block_name()));
        const ::llvm::BasicBlock *BB = BBMap.at(CBB.basic_block_name());
        BBMeta.emplace_back(BBMetadata{
            .BB = BB, .Id = CBB.id(), .InstStartId = CBB.inst_start_id()});
      };

  std::ifstream IFS(UIDFile);
  stream::for_each(IFS, ParseProtobuf);
  IFS.close();

  // Initialize BB & instructions according to the BBMeta.
  InstId CurInstId = FirstInstId;
  BBId CurBBId = FirstBBId;
  for (const BBMetadata &Meta : BBMeta) {
    assert(CurBBId == Meta.Id);
    assert(CurInstId == Meta.InstStartId);

    BBToId[Meta.BB] = CurBBId;
    CurBBId++;

    for (const ::llvm::Instruction &I : *Meta.BB) {
      InstToId[&I] = CurInstId;
      CurInstId++;
    }
  }

  buildReverseMaps();
}

CanonicalId::CanonicalId(const ::llvm::Module *const M,
                         std::filesystem::path UIDFile)
    : CanonicalId(*M, UIDFile) {}

InstId CanonicalId::instId(const ::llvm::Instruction &I) const {
  return instId(&I);
}
InstId CanonicalId::instId(const ::llvm::Instruction *const I) const {
  assert(InstToId.count(I) && "Invalid inst");
  return InstToId.at(I);
}
BBId CanonicalId::bbId(const ::llvm::BasicBlock &BB) const { return bbId(&BB); }

BBId CanonicalId::bbId(const ::llvm::BasicBlock *const BB) const {
  assert(BBToId.count(BB) && "Invalid BB");
  return BBToId.at(BB);
}

const ::llvm::Instruction *CanonicalId::getInst(InstId Id) const {
  assert(hasInst(Id) && "Invalid inst id");
  return IdToInst.at(Id);
}

const ::llvm::BasicBlock *CanonicalId::getBB(BBId Id) const {
  assert(hasBB(Id) && "Invalid BB id");
  return IdToBB.at(Id);
}

bool CanonicalId::hasInst(InstId Id) const { return IdToInst.count(Id); }

bool CanonicalId::hasBB(BBId Id) const { return IdToBB.count(Id); }

std::uint64_t CanonicalId::numInsts() const { return InstToId.size(); }

std::uint64_t CanonicalId::numBBs() const { return BBToId.size(); }

void CanonicalId::serialize(std::filesystem::path UIDFile) const {
  std::function<trace::CanonicalBB(uint64_t)> EmitProtobuf =
      [&BBMeta = BBMeta](uint64_t Idx) {
        const BBMetadata &Meta = BBMeta.at(Idx);
        trace::CanonicalBB CBB;
        CBB.set_function_name(getFuncName(Meta.BB->getParent()));
        CBB.set_basic_block_name(getBBName(Meta.BB));
        CBB.set_id(Meta.Id);
        CBB.set_inst_start_id(Meta.InstStartId);
        return CBB;
      };

  std::ofstream OFS(UIDFile);
  stream::write(OFS, BBMeta.size(), EmitProtobuf);
  OFS.close();
}

std::string CanonicalId::getFuncName(const ::llvm::Function &F) {
  return getFuncName(&F);
}

std::string CanonicalId::getFuncName(const ::llvm::Function *const F) {
  return F->getName().str();
}

std::string CanonicalId::getBBName(const ::llvm::BasicBlock &BB) {
  return getBBName(&BB);
}

std::string CanonicalId::getBBName(const ::llvm::BasicBlock *const BB) {
  std::string BBName;
  ::llvm::raw_string_ostream SS(BBName);
  BB->printAsOperand(SS, false);
  return SS.str();
}

void CanonicalId::buildReverseMaps() {
  for (const auto &[Inst, Id] : InstToId) {
    IdToInst[Id] = Inst;
  }
  for (const auto &[BB, Id] : BBToId) {
    IdToBB[Id] = BB;
  }
}

} // namespace llvm
} // namespace dragongem
