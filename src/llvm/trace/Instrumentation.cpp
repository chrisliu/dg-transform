#include "Instrumentation.h"
#include "stream.hpp"
#include "trace/BBInterval.pb.h"
#include "trace/InstTrace.pb.h"
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <google/protobuf/util/json_util.h>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <unordered_map>

namespace dragongem {
namespace trace {

const std::string ENV_BB_INTERVAL_SIZE = "DG_BB_INTERVAL_SIZE";
const std::string ENV_BB_INTERVAL_PATH = "DG_BB_INTERVAL_PATH";
const std::string ENV_TRACE_PATH = "DG_TRACE_PATH";
const std::string ENV_INST_MAX = "DG_INST_MAX";

class TraceContext {
protected:
  enum Mode { SimPoint, Trace };
  const Mode Mode_;

public:
  static TraceContext *getInstance();

  void incDynamicInstCount();
  std::uint64_t getDynamicInstCount() const;

  void recordBasicBlock(BBId Id);
  void recordLoadInst(InstId Id, void *Address);
  void recordStoreInst(InstId Id, void *Address);

protected:
  static TraceContext *Singleton_;

  const std::optional<std::uint64_t> InstInterval_;
  const std::optional<std::filesystem::path> BBIntervalPath_;
  const std::optional<std::filesystem::path> TracePath_;
  std::optional<std::ofstream> TraceOFS_;
  const std::uint64_t InstMax_;

  std::unordered_map<BBId, std::uint64_t> BBVec_;
  std::uint64_t InstCount_;

  TraceContext();

  void recordBBEnterEvent(BBId Id);

  void saveIntervalBBVec(std::uint64_t InstStart);
  void saveTraceEvent(TraceEvent &TE);

protected:
  static Mode getMode();
  static std::optional<std::uint64_t> getInstInterval();
  static std::optional<std::filesystem::path> getBBIntervalPath();
  static std::optional<std::filesystem::path> getTracePath();
  static std::optional<std::ofstream>
  getTraceOFS(const std::optional<std::filesystem::path> &TracePath);
  static std::uint64_t getInstMax();
};

TraceContext *TraceContext::Singleton_ = nullptr;

TraceContext::TraceContext()
    : Mode_(getMode()), InstInterval_(getInstInterval()),
      BBIntervalPath_(getBBIntervalPath()), TracePath_(getTracePath()),
      TraceOFS_(getTraceOFS(TracePath_)), InstMax_(getInstMax()),
      InstCount_{0} {}

TraceContext *TraceContext::getInstance() {
  if (Singleton_ == nullptr) {
    Singleton_ = new TraceContext();
  }
  return Singleton_;
}

void TraceContext::incDynamicInstCount() {
  InstCount_++;
  if (InstCount_ == InstMax_) {
    std::exit(EXIT_SUCCESS);
  }

  switch (Mode_) {
  case SimPoint:
    if (InstCount_ % *InstInterval_ == 0) {
      saveIntervalBBVec(InstCount_ - *InstInterval_);
    }
    break;
  case Trace:
    break;
  default:
    assert(false && "Unsupported trace context mode");
    break;
  }
}
std::uint64_t TraceContext::getDynamicInstCount() const { return InstCount_; }

void TraceContext::recordBasicBlock(BBId Id) {
  switch (Mode_) {
  case SimPoint:
    BBVec_[Id]++;
    break;
  case Trace:
    recordBBEnterEvent(Id);
    break;
  default:
    assert(false && "Unspported trace context mode");
    break;
  }
}

void TraceContext::recordLoadInst(InstId Id, void *Address) {
  uint64_t AddressU64 = reinterpret_cast<uint64_t>(Address);

  TraceEvent Load;
  TraceEvent::DynamicInst *Inst = Load.mutable_inst();
  Inst->set_inst_id(Id);
  Inst->mutable_memory()->set_address(AddressU64);
  saveTraceEvent(Load);
}

void TraceContext::recordStoreInst(InstId Id, void *Address) {
  uint64_t AddressU64 = reinterpret_cast<uint64_t>(Address);

  TraceEvent Store;
  TraceEvent::DynamicInst *Inst = Store.mutable_inst();
  Inst->set_inst_id(Id);
  Inst->mutable_memory()->set_address(AddressU64);
  saveTraceEvent(Store);
}

void TraceContext::recordBBEnterEvent(BBId Id) {
  TraceEvent BBEnter;
  BBEnter.mutable_bb()->set_bb_id(Id);
  saveTraceEvent(BBEnter);
}

void TraceContext::saveIntervalBBVec(std::uint64_t InstStart) {
  BBInterval BBI;
  BBI.set_inst_start(InstStart);
  BBI.set_inst_end(InstCount_);
  BBI.mutable_freq()->insert(BBVec_.begin(), BBVec_.end());
  // BBI.PrintDebugString();

  std::ofstream OFS(*BBIntervalPath_, std::ofstream::app);
  std::string Json;
  auto status = google::protobuf::util::MessageToJsonString(BBI, &Json);
  if (status.ok()) {
    OFS << Json << std::endl;
    std::cout << Json << std::endl;
    std::cout << *InstInterval_ << std::endl;
    std::cout << *BBIntervalPath_ << std::endl;
  }
  OFS.close();

  BBVec_.clear();
}

void TraceContext::saveTraceEvent(TraceEvent &TE) {
  std::function<TraceEvent(uint64_t)> EmitTE = [&TE](uint64_t I) { return TE; };
  stream::write(*TraceOFS_, 1, EmitTE);
  TraceOFS_->flush();
  // TE.PrintDebugString();
}

TraceContext::Mode TraceContext::getMode() {
  char *InstInterval = std::getenv(ENV_BB_INTERVAL_SIZE.c_str());
  char *BBIntervalPath = std::getenv(ENV_BB_INTERVAL_PATH.c_str());
  char *TracePath = std::getenv(ENV_TRACE_PATH.c_str());
  if (InstInterval && BBIntervalPath) {
    return SimPoint;
  } else if (TracePath) {
    return Trace;
  }
  assert(false && "Could not deterimne trace mode");
}

std::optional<std::uint64_t> TraceContext::getInstInterval() {
  char *InstInterval = std::getenv(ENV_BB_INTERVAL_SIZE.c_str());
  if (InstInterval) {
    return std::stoul(InstInterval);
  }
  return std::nullopt;
}

std::optional<std::filesystem::path> TraceContext::getBBIntervalPath() {
  char *BBIntervalPath = std::getenv(ENV_BB_INTERVAL_PATH.c_str());
  if (BBIntervalPath) {
    return BBIntervalPath;
  }
  return std::nullopt;
}

std::optional<std::filesystem::path> TraceContext::getTracePath() {
  char *TracePath = std::getenv(ENV_TRACE_PATH.c_str());
  if (TracePath) {
    return TracePath;
  }
  return std::nullopt;
}

std::optional<std::ofstream> TraceContext::getTraceOFS(
    const std::optional<std::filesystem::path> &TracePath) {
  if (TracePath.has_value()) {
    return std::ofstream(*TracePath);
  }
  return std::nullopt;
}

std::uint64_t TraceContext::getInstMax() {
  char *InstMax = std::getenv(ENV_INST_MAX.c_str());
  if (InstMax) {
    return std::stoul(InstMax);
  } else {
    return std::numeric_limits<std::uint64_t>::max();
  }
}

} // namespace trace
} // namespace dragongem

void incDynamicInstCount() {
  dragongem::trace::TraceContext::getInstance()->incDynamicInstCount();
}

void recordBasicBlock(BBId Id) {
  dragongem::trace::TraceContext::getInstance()->recordBasicBlock(Id);
}

void recordLoadInst(InstId Id, void *Address) {
  dragongem::trace::TraceContext::getInstance()->recordLoadInst(Id, Address);
}

void recordStoreInst(InstId Id, void *Address) {
  dragongem::trace::TraceContext::getInstance()->recordStoreInst(Id, Address);
}
