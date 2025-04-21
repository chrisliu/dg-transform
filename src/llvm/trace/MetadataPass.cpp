#include "MetadataPass.h"

#include "CLOpts.h"
#include "dragongem/llvm/CanonicalId.h"
#include "dragongem/llvm/Metadata.h"
#include "llvm/ADT/SCCIterator.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/CallGraphSCCPass.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Format.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#define DEBUG_TYPE "dg-metadata-pass"

namespace dragongem {
namespace llvm {
namespace trace {

namespace {
template <std::size_t N>
std::string_view::size_type
getMaxLenSV(const std::array<std::string_view, N> &SVs) {
  return std::max_element(
             SVs.begin(), SVs.end(),
             [](const std::string_view &A, const std::string_view &B) {
               return A.size() < B.size();
             })
      ->size();
}

} // namespace

const std::string MetadataPass::PassName = "dg-metadata-pass";

bool MetadataPass::isTrackedFunction(const ::llvm::Function *const F) {
  return !F->isIntrinsic();
}

bool MetadataPass::isTrackedCall(const ::llvm::Instruction *const I) {
  if (const auto Call = ::llvm::dyn_cast<::llvm::CallBase>(I)) {
    // Case: Inline assembly
    if (Call->isInlineAsm()) {
      return false;
    }

    // Case: Direct call
    if (const auto *const Callee = Call->getCalledFunction()) {
      return isTrackedFunction(Callee);
    }

    // Case: Indirect call
    return true;
  }

  return false;
}

bool MetadataPass::isTrackedCall(const ::llvm::Instruction &I) {
  return isTrackedCall(&I);
}

::llvm::PreservedAnalyses MetadataPass::run(::llvm::Module &M,
                                            ::llvm::ModuleAnalysisManager &AM) {

  assert(!UIDFile.empty() && "Must provide the LLVM UID file");
  assert(!MetadataFile.empty() && "Must provide the LLVM UID file");

  const auto [FMeta, IndRecCalls] = analyzeFunctions(M, AM);
  const auto LMeta = analyzeLoops(M, AM, FMeta, IndRecCalls);

  printStats(FMeta, LMeta);
  serialize(M, AM, FMeta, LMeta);

  return ::llvm::PreservedAnalyses::none();
}

std::pair<FunctionMetadataMap, MetadataPass::CallSet>
MetadataPass::analyzeFunctions(::llvm::Module &M,
                               ::llvm::ModuleAnalysisManager &MAM) {
  FunctionMetadataMap FMeta;

  // Step 1: Identify leafs.
  for (const ::llvm::Function &F : M) {
    if (F.isDeclaration()) {
      continue;
    }

    bool IsNonLeaf = false;

    for (const ::llvm::BasicBlock &BB : F) {
      for (const ::llvm::Instruction &I : BB) {
        if (isTrackedCall(I)) {
          const auto *const Call = ::llvm::cast<::llvm::CallBase>(&I);
          if (::llvm::isa<::llvm::CallInst, ::llvm::InvokeInst>(Call)) {
            IsNonLeaf |= Call->isIndirectCall() ||
                         !Call->getCalledFunction()->isDeclaration();
          } else {
            assert(false && "Not supported");
          }
        }
      }
    }

    FMeta[&F].type =
        IsNonLeaf ? FunctionMetadata::None : FunctionMetadata::Leaf;
  }

  // Step 2: Annotate calls based on call graph.
  // CallGraph SCC iteration example (CGPassManager)
  // https://github.com/llvm/llvm-project/blob/b42f8ec26d5098128b32cc12b5daf342d26fd42e/llvm/lib/Analysis/CallGraphSCCPass.cpp#L500
  ::llvm::CallGraph &CG = MAM.getResult<::llvm::CallGraphAnalysis>(M);
  ::llvm::scc_iterator<::llvm::CallGraph *> CGI = ::llvm::scc_begin(&CG);
  ::llvm::CallGraphSCC CurSCC(CG, &CGI);

  CallSet IndRecCalls;

  while (!CGI.isAtEnd()) {
    const std::vector<::llvm::CallGraphNode *> &NodeVec = *CGI;
    CurSCC.initialize(NodeVec);
    ++CGI;

    if (CurSCC.size() > 1) {
      const std::unordered_set<::llvm::CallGraphNode *> NodeSet(CurSCC.begin(),
                                                                CurSCC.end());

      for (const ::llvm::CallGraphNode *const CGNode : CurSCC) {
        const ::llvm::Function *const F = CGNode->getFunction();
        assert(!CGNode->getFunction()->isDeclaration());
        FMeta[F].type = FunctionMetadata::IndirectRecursiveStatic;

        for (const auto &[MaybeCall, CalledNode] : *CGNode) {
          if (CalledNode != CGNode && NodeSet.count(CalledNode)) {
            assert(MaybeCall);
            IndRecCalls.insert(::llvm::cast<::llvm::CallBase>(*MaybeCall));
          }
        }
      }
    }
  }

  // Step 3: Annotate calls & determine type for non-leaf functions.
  for (const ::llvm::Function &F : M) {
    if (F.isDeclaration()) {
      continue;
    }

    for (const ::llvm::BasicBlock &BB : F) {
      for (const ::llvm::Instruction &I : BB) {
        if (isTrackedCall(I)) {
          const auto *const Call = ::llvm::cast<::llvm::CallBase>(&I);
          recordCall(*Call, FMeta[&F].call, FMeta, IndRecCalls);
        }
      }
    }

    if (FMeta[&F].call.count[CallCount::DirectSelf] > 0) {
      FMeta[&F].type = FunctionMetadata::DirectRecursiveStatic;
    }
  }

  return std::make_pair(FMeta, IndRecCalls);
}

LoopMetadataMap MetadataPass::analyzeLoops(::llvm::Module &M,
                                           ::llvm::ModuleAnalysisManager &MAM,
                                           const FunctionMetadataMap &FMeta,
                                           const CallSet &IndRecCalls) {

  ::llvm::FunctionAnalysisManager &FAM =
      MAM.getResult<::llvm::FunctionAnalysisManagerModuleProxy>(M).getManager();

  std::unordered_map<const ::llvm::Loop *, LoopMetadata> LMeta;

  for (::llvm::Function &F : M) {
    if (F.isDeclaration()) {
      continue;
    }

    ::llvm::LoopInfo &LI = FAM.getResult<::llvm::LoopAnalysis>(F);

    for (const ::llvm::Loop *const L : LI) {
      analyzeLoop(L, LMeta, LI, FMeta, IndRecCalls);
    }
  }

  return LMeta;
}

void MetadataPass::analyzeLoop(const ::llvm::Loop *const L,
                               LoopMetadataMap &LMeta,
                               const ::llvm::LoopInfo &LI,
                               const FunctionMetadataMap &FMeta,
                               const CallSet &IndRecCalls) {

  using LoopTy = LoopMetadata::Type;

  LoopTy Ty = LoopMetadata::InlineLocal;

  // Step 1: Take the most conservative type from the children.
  // e.g. if a child InlineLeaf, this loop must also be at least InlineLeaf.
  for (const ::llvm::Loop *const SL : *L) {
    analyzeLoop(SL, LMeta, LI, FMeta, IndRecCalls);
    Ty = LoopTy{std::max(LMeta[SL].type, Ty)};
  }

  // Step 2: Determine type based on this loop's calls (if any).
  for (const ::llvm::BasicBlock *const BB : L->blocks()) {
    if (LI.getLoopFor(BB) != L) {
      continue;
    }

    for (const ::llvm::Instruction &I : *BB) {
      if (isTrackedCall(I)) {
        const auto *const Call = ::llvm::cast<::llvm::CallBase>(&I);
        recordCall(*Call, LMeta[L].call, FMeta, IndRecCalls);

        LoopTy CallTy = LoopMetadata::None;
        if (const ::llvm::Function *const Callee = Call->getCalledFunction()) {
          if (!Callee->isDeclaration()) {
            if (FMeta.at(Callee).type == FunctionMetadata::Leaf) {
              CallTy = LoopMetadata::InlineLeaf;
            } else if (Callee == Call->getCaller()) {
              CallTy = LoopMetadata::InlineDirectRecursive;
            }
          }
        }
        Ty = LoopTy{std::max(CallTy, Ty)};
      }
    }
  }

  LMeta[L].type = Ty;
}

void MetadataPass::recordCall(const ::llvm::CallBase &Call,
                              CallCount &ThisCallCount,
                              const FunctionMetadataMap &FMeta,
                              const CallSet &IndRecCalls) {
  if (::llvm::isa<::llvm::CallInst, ::llvm::InvokeInst>(Call)) {
    if (IndRecCalls.count(&Call)) {
      assert(!Call.isIndirectCall());
      ++ThisCallCount.count[CallCount::DirectIndRec];
      return;
    }

    if (Call.isIndirectCall()) {
      ++ThisCallCount.count[CallCount::Indirect];
    } else {
      const ::llvm::Function *const Callee = Call.getCalledFunction();
      if (Callee->isDeclaration()) {
        ++ThisCallCount.count[CallCount::DirectExt];
      } else if (FMeta.at(Callee).type == FunctionMetadata::Leaf) {
        ++ThisCallCount.count[CallCount::Leaf];
      } else if (Callee == Call.getCaller()) {
        ++ThisCallCount.count[CallCount::DirectSelf];
      } else {
        ++ThisCallCount.count[CallCount::DirectTU];
      }
    }
  }
}

void MetadataPass::printStats(const FunctionMetadataMap &FMeta,
                              const LoopMetadataMap &LMeta) {
  // 1. Aggregate stats.
  std::array<std::uint64_t, FunctionMetadata::NumTypes> FuncTyCount{};
  std::array<std::uint64_t, CallCount::NumTypes> FuncCallTyCount{};

  for (const auto &[_, Meta] : FMeta) {
    ++FuncTyCount[Meta.type];

    for (auto Ty = 0; Ty < CallCount::NumTypes; ++Ty) {
      FuncCallTyCount[Ty] += Meta.call.count[Ty];
    }
  }

  std::array<std::uint64_t, LoopMetadata::NumTypes> LoopTyCount{};
  std::array<std::uint64_t, CallCount::NumTypes> LoopCallTyCount{};
  for (const auto &[_, Meta] : LMeta) {
    ++LoopTyCount[Meta.type];

    for (auto Ty = 0; Ty < CallCount::NumTypes; ++Ty) {
      LoopCallTyCount[Ty] += Meta.call.count[Ty];
    }
  }

  // 2. Print results.
  constexpr unsigned CountW = 5;

  {
    const auto MaxW = getMaxLenSV(FunctionMetadata::TypeName);
    ::llvm::dbgs() << "Function Types\n";
    auto Total = 0;
    for (auto Ty = 0; Ty < FunctionMetadata::NumTypes; ++Ty) {
      ::llvm::dbgs() << "  - "
                     << ::llvm::left_justify(FunctionMetadata::TypeName[Ty],
                                             MaxW)
                     << ": " << ::llvm::format_decimal(FuncTyCount[Ty], CountW)
                     << "\n";
      Total += FuncTyCount[Ty];
    }
    ::llvm::dbgs() << "  - " << ::llvm::left_justify("Total", MaxW) << ": "
                   << ::llvm::format_decimal(Total, CountW) << "\n";
  }

  {
    const auto MaxW = getMaxLenSV(CallCount::TypeName);
    ::llvm::dbgs() << "Function Call Types\n";
    auto Total = 0;
    for (auto Ty = 0; Ty < CallCount::NumTypes; ++Ty) {
      ::llvm::dbgs() << "  - "
                     << ::llvm::left_justify(CallCount::TypeName[Ty], MaxW)
                     << ": "
                     << ::llvm::format_decimal(FuncCallTyCount[Ty], CountW)
                     << "\n";
      Total += FuncCallTyCount[Ty];
    }
    ::llvm::dbgs() << "  - " << ::llvm::left_justify("Total", MaxW) << ": "
                   << ::llvm::format_decimal(Total, CountW) << "\n";
  }

  {
    const auto MaxW = getMaxLenSV(LoopMetadata::TypeName);
    ::llvm::dbgs() << "Loop Types\n";
    auto Total = 0;
    for (auto Ty = 0; Ty < LoopMetadata::NumTypes; ++Ty) {
      ::llvm::dbgs() << "  - "
                     << ::llvm::left_justify(LoopMetadata::TypeName[Ty], MaxW)
                     << ": " << ::llvm::format_decimal(LoopTyCount[Ty], CountW)
                     << "\n";
      Total += LoopTyCount[Ty];
    }
    ::llvm::dbgs() << "  - " << ::llvm::left_justify("Total", MaxW) << ": "
                   << ::llvm::format_decimal(Total, CountW) << "\n";
  }

  {
    const auto MaxW = getMaxLenSV(CallCount::TypeName);
    ::llvm::dbgs() << "Loop Call Types\n";
    auto Total = 0;
    for (auto Ty = 0; Ty < CallCount::NumTypes; ++Ty) {
      ::llvm::dbgs() << "  - "
                     << ::llvm::left_justify(CallCount::TypeName[Ty], MaxW)
                     << ": "
                     << ::llvm::format_decimal(LoopCallTyCount[Ty], CountW)
                     << "\n";
      Total += LoopCallTyCount[Ty];
    }
    ::llvm::dbgs() << "  - " << ::llvm::left_justify("Total", MaxW) << ": "
                   << ::llvm::format_decimal(Total, CountW) << "\n";
  }

  const FunctionMetadataMap IsoFMeta = analysis::isolateNonLoop(FMeta, LMeta);

  std::array<std::uint64_t, FunctionMetadata::NumTypes> IsoFuncTyCount{};
  std::array<std::uint64_t, CallCount::NumTypes> IsoFuncCallTyCount{};

  for (const auto &[_, Meta] : IsoFMeta) {
    ++IsoFuncTyCount[Meta.type];

    for (auto Ty = 0; Ty < CallCount::NumTypes; ++Ty) {
      IsoFuncCallTyCount[Ty] += Meta.call.count[Ty];
    }
  }

  {
    const auto MaxW = getMaxLenSV(FunctionMetadata::TypeName);
    ::llvm::dbgs() << "Function Types\n";
    auto Total = 0;
    for (auto Ty = 0; Ty < FunctionMetadata::NumTypes; ++Ty) {
      ::llvm::dbgs() << "  - "
                     << ::llvm::left_justify(FunctionMetadata::TypeName[Ty],
                                             MaxW)
                     << ": "
                     << ::llvm::format_decimal(IsoFuncTyCount[Ty], CountW)
                     << "\n";
      Total += IsoFuncTyCount[Ty];
    }
    ::llvm::dbgs() << "  - " << ::llvm::left_justify("Total", MaxW) << ": "
                   << ::llvm::format_decimal(Total, CountW) << "\n";
  }

  {
    const auto MaxW = getMaxLenSV(CallCount::TypeName);
    ::llvm::dbgs() << "Function Call Types\n";
    auto Total = 0;
    for (auto Ty = 0; Ty < CallCount::NumTypes; ++Ty) {
      ::llvm::dbgs() << "  - "
                     << ::llvm::left_justify(CallCount::TypeName[Ty], MaxW)
                     << ": "
                     << ::llvm::format_decimal(IsoFuncCallTyCount[Ty], CountW)
                     << "\n";
      Total += IsoFuncCallTyCount[Ty];
    }
    ::llvm::dbgs() << "  - " << ::llvm::left_justify("Total", MaxW) << ": "
                   << ::llvm::format_decimal(Total, CountW) << "\n";
  }
}

void MetadataPass::serialize(::llvm::Module &M,
                             ::llvm::ModuleAnalysisManager &MAM,
                             const FunctionMetadataMap &FMeta,
                             const LoopMetadataMap &LMeta) {

  ::llvm::FunctionAnalysisManager &FAM =
      MAM.getResult<::llvm::FunctionAnalysisManagerModuleProxy>(M).getManager();
  const CanonicalId CID(M, FAM);
  CID.serialize(UIDFile);

  MetadataSerDe::Serialize(MetadataFile, CID, FMeta, LMeta);
}

} // namespace trace
} // namespace llvm
} // namespace dragongem
