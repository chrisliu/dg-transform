#ifndef DRAGONGEM_LLVM_METADATA_H
#define DRAGONGEM_LLVM_METADATA_H

#include <array>
#include <cstdint>
#include <filesystem>
#include <string_view>
#include <unordered_map>

#include "dragongem/llvm/CanonicalId.h"
#include "dragongem/trace/Metadata.pb.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Function.h"

namespace dragongem {
namespace llvm {

struct CallCount {
  enum Type : std::int16_t {
    Leaf,
    DirectIndRec,
    DirectSelf,
    DirectTU,  // In translation unit.
    DirectExt, // Not in translation unit.
    Indirect,
    NumTypes
  };
  static constexpr std::array<std::string_view, NumTypes> TypeName = {
      "Leaf",            //
      "Direct (IndRec)", // Direct calls that results in indirect recursion.
      "Direct (Self)",   // Direct calls that results in direct recursion.
      "Direct (TU)",     //
      "Direct (Ext)",    //
      "Indirect",        //
  };

  std::array<std::int32_t, NumTypes> count{};

  CallCount() = default;
  CallCount(const proto::CallCount &pb_call_count);

  std::int32_t GetTotalCount() const;

  proto::CallCount AsProto() const;

  CallCount &operator+=(const CallCount &rhs);
  CallCount &operator-=(const CallCount &rhs);
  // passing lhs by value helps optimize chained a+b+c
  friend CallCount operator+(CallCount lhs, const CallCount &rhs);
  friend CallCount operator-(CallCount lhs, const CallCount &rhs);
};

struct FunctionMetadata {
  // Strict Inequality: None > DirectRecursiveStatic > ... > Leaf
  enum Type : std::int16_t {
    Leaf,                     // Has no calls.
    IndirectRecursiveDynamic, // Indirectly calls itself (determined
                              // dynamically).
    IndirectRecursiveStatic,  // Indirectly calls itself (determined
                              // statically).
    DirectRecursiveDynamic,   // Directly calls itself (statically).
    DirectRecursiveStatic,    // Directly calls itself (statically).
    None,
    NumTypes
  };
  static constexpr std::array<std::string_view, NumTypes> TypeName = {
      "Leaf",
      "Indirect Recursive (Dynamic)",
      "Indirect Recursive (Static)",
      "Direct Recursive (Dynamic)",
      "Direct Recursive (Static)",
      "None",
  };

  CallCount call{};
  Type type = NumTypes;

  FunctionMetadata() = default;
  FunctionMetadata(const proto::Function &pb_function);

  proto::Function AsProto(const ::llvm::Function *const function,
                          const CanonicalId &canon_id) const;
};

struct LoopMetadata {
  // Strict Inequality: None > InlineFunction > InlineLocal
  enum Type : std::int16_t {
    InlineLocal,           // Doesn't make any calls.
    InlineLeaf,            // Only calls leaf functions.
    InlineDirectRecursive, // Only calls itself.
    None,
    NumTypes
  };
  static constexpr std::array<std::string_view, NumTypes> TypeName = {
      "Inline (Local)",
      "Inline (Leaf)",
      "Inline (Direct Recursive)",
      "None",
  };

  CallCount call{};
  Type type = NumTypes;

  LoopMetadata() = default;
  LoopMetadata(const proto::Loop &pb_loop);

  proto::Loop AsProto(const ::llvm::Loop *const loop,
                      const CanonicalId &canon_id) const;
};

using FunctionMetadataMap =
    std::unordered_map<const ::llvm::Function *, FunctionMetadata>;
using LoopMetadataMap = std::unordered_map<const ::llvm::Loop *, LoopMetadata>;

struct MetadataSerDe {

  static void Serialize(const std::filesystem::path metadata_file,
                        const CanonicalId &canon_id,
                        const FunctionMetadataMap &fmeta,
                        const LoopMetadataMap &lmeta);
  static void Deserialize(const std::filesystem::path metadata_file,
                          const CanonicalId &canon_id,
                          FunctionMetadataMap &fmeta, LoopMetadataMap &lmeta);
};

namespace analysis {

bool isValidCallCount(const CallCount &call_count);

FunctionMetadataMap isolateNonLoop(const FunctionMetadataMap &fmeta,
                                   const LoopMetadataMap &lmeta);
} // namespace analysis

} // namespace llvm
} // namespace dragongem

#endif
