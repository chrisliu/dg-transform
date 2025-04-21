#include "dragongem/llvm/CanonicalId.h"

#include "dragongem/llvm/ExecutableBasicBlock.h"
#include "dragongem/trace/LLVMUID.pb.h"
#include "stream.hpp"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Support/raw_ostream.h"

#include <cassert>
#include <cstdint>
#include <fstream>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

namespace dragongem {
namespace llvm {

CanonicalId::CanonicalId(const ::llvm::Module &M,
                         ::llvm::FunctionAnalysisManager &FAM) {
  InstId CurInstId = FirstInstId;
  BBId CurBBId = FirstBBId;
  LoopId CurLoopId = FirstLoopId;

  for (const ::llvm::Function &F : M) {
    // Handle loops.
    std::unordered_map<const ::llvm::BasicBlock *, LoopId> LoopHeader;

    if (!F.isDeclaration()) {
      ::llvm::Function *const MFunc = const_cast<::llvm::Function *>(&F);
      ::llvm::LoopInfo &LI = FAM.getResult<::llvm::LoopAnalysis>(*MFunc);
      for (const ::llvm::Loop *L : LI.getLoopsInPreorder()) {
        LoopToId[L] = CurLoopId;
        LoopHeader[L->getHeader()] = CurLoopId;
        ++CurLoopId;
      }
    }

    // Handle BB and insts.
    for (const ::llvm::BasicBlock &BB : F) {
      std::optional<LoopId> ThisLoopId;
      if (LoopHeader.count(&BB)) {
        ThisLoopId = LoopHeader.at(&BB);
      }
      BBMeta.emplace_back(BBMetadata{.BB = &BB,
                                     .Id = CurBBId,
                                     .InstStartId = CurInstId,
                                     .ThisLoopId = ThisLoopId});
      BBToId[&BB] = CurBBId;
      ++CurBBId;

      for (const ::llvm::Instruction &I : BB) {
        InstToId[&I] = CurInstId;
        ++CurInstId;
      }
    }
  }

  buildReverseMaps();
}

CanonicalId::CanonicalId(const ::llvm::Module *const M,
                         ::llvm::FunctionAnalysisManager &FAM)
    : CanonicalId(*M, FAM) {}

CanonicalId::CanonicalId(const ::llvm::Module &M,
                         ::llvm::FunctionAnalysisManager &FAM,
                         const std::filesystem::path UIDFile) {
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
      [&BBMeta = BBMeta, &FuncNameToBBMap = std::as_const(FuncNameToBBMap),
       &FAM = FAM](trace::CanonicalBB &CBB) {
        assert(FuncNameToBBMap.count(CBB.function_name()));
        const NameToBBMap &BBMap = FuncNameToBBMap.at(CBB.function_name());
        assert(BBMap.count(CBB.basic_block_name()));
        const ::llvm::BasicBlock *BB = BBMap.at(CBB.basic_block_name());
        std::optional<LoopId> ThisLoopId;
        if (CBB.has_loop_id()) {
          ThisLoopId = CBB.loop_id();
        }

        BBMeta.emplace_back(BBMetadata{
            .BB = BB,
            .Id = CBB.id(),
            .InstStartId = CBB.inst_start_id(),
            .ThisLoopId = ThisLoopId,
        });
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
    ++CurBBId;

    for (const ::llvm::Instruction &I : *Meta.BB) {
      InstToId[&I] = CurInstId;
      ++CurInstId;
    }

    if (Meta.ThisLoopId) {
      ::llvm::Function *const MFunc =
          const_cast<::llvm::Function *>(Meta.BB->getParent());
      ::llvm::LoopInfo &LI = FAM.getResult<::llvm::LoopAnalysis>(*MFunc);
      const ::llvm::Loop *L = LI.getLoopFor(Meta.BB);
      LoopToId[L] = *Meta.ThisLoopId;
    }
  }

  buildReverseMaps();
}

CanonicalId::CanonicalId(const ::llvm::Module *const M,
                         ::llvm::FunctionAnalysisManager &FAM,
                         const std::filesystem::path UIDFile)
    : CanonicalId(*M, FAM, UIDFile) {}

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

LoopId CanonicalId::loopId(const ::llvm::Loop &L) const { return loopId(&L); }

LoopId CanonicalId::loopId(const ::llvm::Loop *const L) const {
  assert(LoopToId.count(L) && "Invalid loop");
  return LoopToId.at(L);
}

FunctionId CanonicalId::functionId(const ::llvm::Function &F) const {
  return functionId(&F);
}

FunctionId CanonicalId::functionId(const ::llvm::Function *const F) const {
  assert(!F->isDeclaration() && "Hack: only support non defined functions");
  return bbId(F->getEntryBlock());
}

const ::llvm::Instruction *CanonicalId::getInst(InstId Id) const {
  assert(hasInst(Id) && "Invalid inst id");
  return IdToInst.at(Id);
}

const ::llvm::BasicBlock *CanonicalId::getBB(BBId Id) const {
  assert(hasBB(Id) && "Invalid BB id");
  return IdToBB.at(Id);
}

const ::llvm::Loop *CanonicalId::getLoop(LoopId Id) const {
  assert(hasLoop(Id) && "Invalid loop id");
  return IdToLoop.at(Id);
}

const ::llvm::Function *CanonicalId::getFunction(FunctionId Id) const {
  assert(hasFunction(Id) && "Invalid function id");
  return getBB(Id)->getParent();
}

bool CanonicalId::hasInst(InstId Id) const { return IdToInst.count(Id); }

bool CanonicalId::hasBB(BBId Id) const { return IdToBB.count(Id); }

bool CanonicalId::hasLoop(LoopId Id) const { return IdToLoop.count(Id); }

bool CanonicalId::hasFunction(FunctionId Id) const { return hasBB(Id); }

std::uint64_t CanonicalId::numInsts() const { return InstToId.size(); }

std::uint64_t CanonicalId::numBBs() const { return BBToId.size(); }

std::uint64_t CanonicalId::numLoops() const { return LoopToId.size(); }

void CanonicalId::serialize(std::filesystem::path UIDFile) const {
  std::function<trace::CanonicalBB(uint64_t)> EmitProtobuf =
      [&BBMeta = BBMeta](uint64_t Idx) {
        const BBMetadata &Meta = BBMeta.at(Idx);
        trace::CanonicalBB CBB;
        CBB.set_function_name(getFuncName(Meta.BB->getParent()));
        CBB.set_basic_block_name(getBBName(Meta.BB));
        CBB.set_id(Meta.Id);
        CBB.set_inst_start_id(Meta.InstStartId);
        CBB.set_bb_size(getExecutableBasicBlock(*Meta.BB).size());

        if (Meta.ThisLoopId) {
          CBB.set_loop_id(*Meta.ThisLoopId);
        }

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
  for (const auto &[Loop, Id] : LoopToId) {
    IdToLoop[Id] = Loop;
  }
}

} // namespace llvm
} // namespace dragongem
