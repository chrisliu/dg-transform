#include "dragongem/llvm/Metadata.h"

#include "dragongem/llvm/CanonicalId.h"
#include "dragongem/trace/Metadata.pb.h"
#include "stream.hpp"
#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Format.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <functional>
#include <ios>
#include <iterator>
#include <type_traits>

namespace dragongem {
namespace llvm {

namespace {

constexpr std::array<proto::Function::Type, FunctionMetadata::NumTypes>
    kPbFunctionTy = {
        proto::Function::TYPE_Leaf,
        proto::Function::TYPE_IndirectRecursiveDynamic,
        proto::Function::TYPE_IndirectRecursiveStatic,
        proto::Function::TYPE_DirectRecursiveDynamic,
        proto::Function::TYPE_DirectRecursiveStatic,
        proto::Function::TYPE_None,
};

constexpr std::array<proto::Loop::Type, FunctionMetadata::NumTypes> kPbLoopTy =
    {
        proto::Loop::TYPE_InlineLocal,
        proto::Loop::TYPE_InlineLeaf,
        proto::Loop::TYPE_InlineDirectRecursive,
        proto::Loop::TYPE_None,
};

} // namespace

CallCount::CallCount(const proto::CallCount &pb_call_count) {
  count[Leaf] = pb_call_count.num_leaf();
  count[DirectSelf] = pb_call_count.num_direct_self();
  count[DirectTU] = pb_call_count.num_direct_tu();
  count[DirectExt] = pb_call_count.num_direct_ext();
  count[Indirect] = pb_call_count.num_indirect();
}

std::int32_t CallCount::GetTotalCount() const {
  std::int32_t sum;
  for (const auto cnt : count) {
    sum += cnt;
  }
  return sum;
}

proto::CallCount CallCount::AsProto() const {
  proto::CallCount pb_call_count;

  pb_call_count.set_num_leaf(count[Leaf]);
  pb_call_count.set_num_direct_indrec(count[DirectIndRec]);
  pb_call_count.set_num_direct_self(count[DirectSelf]);
  pb_call_count.set_num_direct_tu(count[DirectTU]);
  pb_call_count.set_num_direct_ext(count[DirectExt]);
  pb_call_count.set_num_indirect(count[Indirect]);

  return pb_call_count;
}

CallCount &CallCount::operator+=(const CallCount &rhs) {
  for (auto ty = 0; ty < CallCount::NumTypes; ++ty) {
    count[ty] += rhs.count[ty];
  }
  return *this;
}

CallCount &CallCount::operator-=(const CallCount &rhs) {
  for (auto ty = 0; ty < CallCount::NumTypes; ++ty) {
    count[ty] -= rhs.count[ty];
  }
  return *this;
}

CallCount operator+(CallCount lhs, const CallCount &rhs) {
  lhs += rhs;
  return lhs;
}

CallCount operator-(CallCount lhs, const CallCount &rhs) {
  lhs -= rhs;
  return lhs;
}

FunctionMetadata::FunctionMetadata(const proto::Function &pb_function) {
  call = CallCount(pb_function.call());

  const auto idx =
      std::distance(kPbFunctionTy.begin(),
                    std::find(kPbFunctionTy.begin(), kPbFunctionTy.end(),
                              pb_function.type()));
  assert(idx < kPbFunctionTy.size());
  type = static_cast<Type>(idx);
}

proto::Function
FunctionMetadata::AsProto(const ::llvm::Function *const function,
                          const CanonicalId &canon_id) const {
  assert(type != NumTypes);

  proto::Function pb_function;

  pb_function.set_id(canon_id.functionId(function));
  pb_function.set_name(CanonicalId::getFuncName(function));
  pb_function.set_type(kPbFunctionTy[type]);

  *pb_function.mutable_call() = call.AsProto();

  return pb_function;
}

LoopMetadata::LoopMetadata(const proto::Loop &pb_loop) {
  call = CallCount(pb_loop.call());

  const auto idx = std::distance(
      kPbLoopTy.begin(),
      std::find(kPbLoopTy.begin(), kPbLoopTy.end(), pb_loop.type()));
  assert(idx < kPbLoopTy.size());
  type = static_cast<Type>(idx);
}

proto::Loop LoopMetadata::AsProto(const ::llvm::Loop *const loop,
                                  const CanonicalId &canon_id) const {
  assert(type != NumTypes);

  proto::Loop pb_loop;

  pb_loop.set_id(canon_id.loopId(loop));
  pb_loop.set_name(CanonicalId::getBBName(loop->getHeader()));
  pb_loop.set_type(kPbLoopTy[type]);

  pb_loop.set_function_id(canon_id.functionId(loop->getHeader()->getParent()));
  if (const ::llvm::Loop *const parent_loop = loop->getParentLoop()) {
    pb_loop.set_parent_loop_id(canon_id.loopId(parent_loop));
  }

  *pb_loop.mutable_call() = call.AsProto();

  return pb_loop;
}

void MetadataSerDe::Serialize(const std::filesystem::path metadata_file,
                              const CanonicalId &canon_id,
                              const FunctionMetadataMap &fmeta,
                              const LoopMetadataMap &lmeta) {

  std::ofstream ofs(metadata_file);

  {
    auto fm_it = fmeta.cbegin();
    std::function<proto::MetadataEntry(const std::uint64_t)> write_fmeta =
        [&canon_id, &fmeta, &fm_it]([[maybe_unused]] const std::uint64_t idx) {
          assert(fm_it != fmeta.end());
          const auto &[function, meta] = *fm_it;
          ++fm_it;

          proto::MetadataEntry pb_meta_entry;
          *pb_meta_entry.mutable_function() = meta.AsProto(function, canon_id);

          // ::llvm::dbgs() << pb_meta_entry.ShortDebugString() << "\n";
          return pb_meta_entry;
        };
    stream::write(ofs, fmeta.size(), write_fmeta);
  }

  {
    auto lm_it = lmeta.cbegin();
    std::function<proto::MetadataEntry(const std::uint64_t)> write_lmeta =
        [&canon_id, &lmeta, &lm_it]([[maybe_unused]] const std::uint64_t idx) {
          assert(lm_it != lmeta.end());
          const auto &[loop, meta] = *lm_it;
          ++lm_it;

          proto::MetadataEntry pb_meta_entry;
          *pb_meta_entry.mutable_loop() = meta.AsProto(loop, canon_id);

          // ::llvm::dbgs() << pb_meta_entry.ShortDebugString() << "\n";
          return pb_meta_entry;
        };
    stream::write(ofs, lmeta.size(), write_lmeta);
  }

  ofs.close();
}

void MetadataSerDe::Deserialize(const std::filesystem::path metadata_file,
                                const CanonicalId &canon_id,
                                FunctionMetadataMap &fmeta,
                                LoopMetadataMap &lmeta) {

  std::function<void(proto::MetadataEntry &)> populate_meta =
      [&fmeta, &lmeta, &canon_id](proto::MetadataEntry &pb_meta_entry) {
        if (pb_meta_entry.has_function()) {
          const proto::Function &pb_function = pb_meta_entry.function();
          fmeta.try_emplace(canon_id.getFunction(pb_function.id()),
                            pb_function);
        } else if (pb_meta_entry.has_loop()) {
          const proto::Loop &pb_loop = pb_meta_entry.loop();
          lmeta.try_emplace(canon_id.getLoop(pb_loop.id()), pb_loop);
        } else {
          assert(false && "Unsupported entry type");
        }
      };

  std::ifstream ifs(metadata_file, std::ios::binary | std::ios::in);
  stream::for_each(ifs, populate_meta);
  ifs.close();
}

namespace analysis {

bool isValidCallCount(const CallCount &call_count) {
  using CountT = decltype(call_count.count)::value_type;
  static_assert(std::is_signed_v<CountT>,
                "Must be signed to disallow negatives");

  for (auto ty = 0; ty < CallCount::NumTypes; ++ty) {
    if (call_count.count[ty] < 0) {
      return false;
    }
  }
  return true;
}

FunctionMetadataMap isolateNonLoop(const FunctionMetadataMap &fmeta,
                                   const LoopMetadataMap &lmeta) {
  FunctionMetadataMap iso_fmeta(fmeta);

  const auto printCallCount = [&](const CallCount &cc) {
    for (auto ty = 0; ty < CallCount::NumTypes; ++ty) {
      ::llvm::dbgs() << "  - "
                     << ::llvm::left_justify(CallCount::TypeName[ty], 30)
                     << ": " << ::llvm::format_decimal(cc.count[ty], 5) << "\n";
    }
  };

  // Remove all call contributions from all loops.
  for (const auto &[loop, meta] : lmeta) {
    const ::llvm::Function *func = loop->getHeader()->getParent();

    iso_fmeta.at(func).call -= meta.call;
  }

  // Relabel function types if necessary.
  for (auto &[func, meta] : iso_fmeta) {
    assert(isValidCallCount(meta.call));

    if (meta.call.GetTotalCount() == 0) {
      meta.type = FunctionMetadata::Leaf;
    } else {
      if (meta.call.count[CallCount::DirectIndRec] > 0) {
        meta.type = FunctionMetadata::IndirectRecursiveStatic;
      }
      if (meta.call.count[CallCount::DirectSelf] > 0) {
        meta.type = FunctionMetadata::DirectRecursiveStatic;
      }
    }
  }

  return iso_fmeta;
}

} // namespace analysis

} // namespace llvm
} // namespace dragongem
