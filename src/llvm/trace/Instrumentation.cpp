#include "Instrumentation.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "stream.hpp"
#include "trace/BBInterval.pb.h"
#include "trace/InstTrace.pb.h"

// #define ENABLE_DEBUG

namespace dragongem {
namespace trace {

const std::string ENV_MODE = "DG_MODE"; // { SimPoint, InstTrace }

// SimPoint.
const std::string ENV_BB_INTERVAL_SIZE = "DG_BB_INTERVAL_SIZE";
const std::string ENV_BB_INTERVAL_PATH = "DG_BB_INTERVAL_PATH";

// InstTrace.
const std::string ENV_TRACE_PATH = "DG_TRACE_PATH";
const std::string ENV_INST_START = "DG_INST_START";
const std::string ENV_INST_MAX = "DG_INST_MAX";
const std::string ENV_SIMPOINT_PATH = "DG_SIMPOINT_PATH";

using DynInstId = std::uint64_t;

struct InstInterval {
  const DynInstId Start;
  const std::optional<DynInstId> End; // Inclusive.

  InstInterval(const DynInstId Start_) : Start(Start_), End(std::nullopt) {}
  InstInterval(const DynInstId Start_, const DynInstId End_)
      : Start(Start_), End(End_) {}

  bool inInterval(DynInstId id) const {
    return id >= Start && (!End || id <= End.value());
  }

  friend std::ostream &operator<<(std::ostream &OS, const InstInterval &I) {
    OS << "InstInterval [" << I.Start << ", ";
    if (I.End) {
      OS << *I.End;
    } else {
      OS << "inf";
    }
    OS << "]";
    return OS;
  }
};

struct GetEnv {
  static std::ofstream ofstream(const std::string EnvVar);
  static std::filesystem::path path(const std::string EnvVar);
  static DynInstId dynInstId(const std::string EnvVar);
};

// TraceContext
// For each instruction, the convention is:
//   record...()
//   incDynamicInstCount()
//   execute actual instruction
class TraceContext {
public:
  static TraceContext *getInstance();

  virtual void incDynamicInstCount() { ++CurInstId; }

  virtual void recordFunctionEntry() {}
  virtual void recordReturnInst() {}

  virtual void recordBasicBlock(BBId Id) {}
  virtual void recordLandingPad(BBId FunctionId) {}

  virtual void recordLoadInst(InstId Id, void *Address) {}
  virtual void recordStoreInst(InstId Id, void *Address) {}

protected:
  DynInstId curInstId() const { return CurInstId; }

  // void exit(std::string Msg = std::string(), bool IsSucess = true) const;

private:
  DynInstId CurInstId = 0;
  static TraceContext *Singleton; // = nullptr
};

TraceContext *TraceContext::Singleton = nullptr;

class SimPointContext : public TraceContext {
public:
  SimPointContext();

  void incDynamicInstCount() override;
  void recordBasicBlock(BBId Id) override;

protected:
  const DynInstId IntervalSize;
  std::ofstream BBIntervalOFS;

  std::unordered_map<BBId, std::uint64_t> BBVec;
};

class InstTraceContext : public TraceContext {
protected:
  static std::vector<InstInterval> getTraceIntervals();

  class IntervalIterator {
  public:
    IntervalIterator(const std::vector<InstInterval> &TraceIntervals_,
                     const std::filesystem::path OFSPath_);

    bool isDone() const;
    // Advance iterator and returns if it reached the end.
    bool advance();

    std::ofstream &OFS() { return OFS_; }
    const InstInterval &instInterval() const { return *It; }
    bool isEndKnown() const { return It->End.has_value(); };
    bool isWaitingForInterval(DynInstId Id) const { return Id < It->Start; }
    bool inInterval(DynInstId Id) const { return It->inInterval(Id); }

    std::size_t SerializedCount = 0;

  protected:
    const std::vector<InstInterval> &TraceIntervals;
    const std::filesystem::path OFSPath;

    std::vector<InstInterval>::const_iterator It;
    std::ofstream OFS_;
    std::size_t Idx = 0;

    std::filesystem::path getCurOFSPath();

  private:
    friend std::ostream &operator<<(std::ostream &OS,
                                    const IntervalIterator &It) {
      if (It.isDone()) {
        return OS;
      }

      OS << "Interval " << It.Idx << " ";
      OS << "[" << It.It->Start << ", ";
      if (It.It->End) {
        OS << *It.It->End;
      } else {
        OS << "inf";
      }
      OS << "]";
      return OS;
    }
  };

  struct CallFrame {
    const BBId FrameId;
    BBId CurBB;
    DynInstId BBExecCount = 0;

    CallFrame(const BBId CurBB_) : FrameId(CurBB_), CurBB(CurBB_) {}
    void enterBB(BBId Id) {
      CurBB = Id;
      BBExecCount = 0;
    }

    friend std::ostream &operator<<(std::ostream &OS, const CallFrame &F) {
      OS << "CallFrame " << F.FrameId << " " << F.CurBB << ":" << F.BBExecCount;
      return OS;
    }
  };

public:
  InstTraceContext();

  void incDynamicInstCount() override;
  void recordFunctionEntry() override;
  void recordReturnInst() override;
  void recordBasicBlock(BBId Id) override;
  void recordLandingPad(BBId FunctionId) override;
  void recordLoadInst(InstId Id, void *Address) override;
  void recordStoreInst(InstId Id, void *Address) override;

protected:
  const DynInstId SerializeTESize = 1000; // Serialize every X TEs.

  const std::vector<InstInterval> TraceIntervals;
  IntervalIterator CurInterval;

  std::deque<CallFrame> CallStack;
  /*bool WillCall = false;*/
  /*bool ExpectNewCallFrame = true;*/
  struct {
    std::optional<BBId> Id = std::nullopt;
    bool IsFunctionEntry = false;
    std::optional<BBId> LPFuncId = std::nullopt; // Landing pad function id.
                                                 //
    bool isValid() const {
      return (IsFunctionEntry || isLandingPad())
                 ? (IsFunctionEntry ^ isLandingPad()) && Id.has_value()
                 : true;
    }

    bool isLandingPad() const { return LPFuncId.has_value(); }

    void reset() {
      Id = std::nullopt;
      IsFunctionEntry = false;
      LPFuncId = std::nullopt;
    }
  } EnteredBB;
  /*std::optional<BBId> EnteredBB = std::nullopt;*/
  bool WillReturn = false;

  std::vector<TraceEvent> TEs;

  void recordCallStack();
  void trySerializeTEs();
  void serializeTEs();

  void dumpCallStack() const;
};

// Implementations.

std::ofstream GetEnv::ofstream(const std::string EnvVar) {
  char *File = std::getenv(EnvVar.c_str());
  if (File) {
    return std::ofstream(File);
  }
  std::cerr << "Missing output file path for " << EnvVar << std::endl;
  std::exit(EXIT_FAILURE);
}

std::filesystem::path GetEnv::path(const std::string EnvVar) {
  char *File = std::getenv(EnvVar.c_str());
  if (File) {
    return std::filesystem::path(File);
  }
  std::cerr << "Missing file path for " << EnvVar << std::endl;
  std::exit(EXIT_FAILURE);
}

DynInstId GetEnv::dynInstId(const std::string EnvVar) {
  char *Id = std::getenv(EnvVar.c_str());
  if (Id) {
    return std::stoull(Id);
  }
  std::cerr << "Missing dynamic inst id for " << EnvVar << std::endl;
  std::exit(EXIT_FAILURE);
}

TraceContext *TraceContext::getInstance() {
  if (!Singleton) {
    char *ModeC = std::getenv(ENV_MODE.c_str());
    if (ModeC) {
      std::string Mode(ModeC);
      if (Mode == "SimPoint") {
        Singleton = new SimPointContext();
      } else if (Mode == "InstTrace") {
        Singleton = new InstTraceContext();
      }
    }

    if (!Singleton) {
      std::cerr << "Unrecognized instrumentation mode: " << ModeC << std::endl;
      std::exit(EXIT_FAILURE);
    }
  }

  return Singleton;
}

SimPointContext::SimPointContext()
    : IntervalSize(GetEnv::dynInstId(ENV_BB_INTERVAL_SIZE)),
      BBIntervalOFS(GetEnv::ofstream(ENV_BB_INTERVAL_PATH)) {}

void SimPointContext::incDynamicInstCount() {
  TraceContext::incDynamicInstCount();

  if (curInstId() % IntervalSize == 0) {
    BBInterval BBI;
    BBI.set_inst_start(curInstId() - IntervalSize);
    BBI.set_inst_end(curInstId() - 1);
    BBI.mutable_freq()->insert(BBVec.begin(), BBVec.end());

    std::function<BBInterval(uint64_t)> EmitBBI = [&BBI](uint64_t) {
      return BBI;
    };
    stream::write(BBIntervalOFS, 1, EmitBBI);
    BBIntervalOFS.flush();

    BBVec.clear();
  }
}

void SimPointContext::recordBasicBlock(BBId Id) { ++BBVec[Id]; }

std::vector<InstInterval> InstTraceContext::getTraceIntervals() {
  std::vector<InstInterval> TraceIntervals;

  {
    // Handle if either or both INST_START and INST_MAX has been defined.
    char *InstStartStr = std::getenv(ENV_INST_START.c_str());
    DynInstId InstStart = InstStartStr ? std::stoull(InstStartStr) : 0;

    char *InstMaxStr = std::getenv(ENV_INST_MAX.c_str());
    if (InstMaxStr) {
      DynInstId InstMax = std::stoull(InstMaxStr);
      TraceIntervals.emplace_back(InstStart, InstStart + InstMax - 1);
      return TraceIntervals;
    } else if (InstStartStr) {
      TraceIntervals.emplace_back(InstStart);
      return TraceIntervals;
    }
  }

  {
    char *SimPointPathStr = std::getenv(ENV_SIMPOINT_PATH.c_str());
    if (SimPointPathStr) {
      std::filesystem::path SimPointPath(SimPointPathStr);
      assert(std::filesystem::exists(SimPointPath));

      std::ifstream IFS(SimPointPath);
      std::string Line;
      while (std::getline(IFS, Line)) {
        if (Line.empty()) {
          continue;
        }

        std::vector<DynInstId> Ints;
        std::stringstream SS(Line);
        for (DynInstId I; SS >> I;) {
          Ints.push_back(I);
          if (SS.peek() == ',') {
            SS.ignore();
          }
        }

        std::cout << Line << std::endl;
        assert(Ints.size() == 3 && "Invalid SimPoints file format");
        TraceIntervals.emplace_back(Ints[0], Ints[1]);
      }
      return TraceIntervals;
    }
  }

  TraceIntervals.emplace_back(0);
  return TraceIntervals;
}

InstTraceContext::IntervalIterator::IntervalIterator(
    const std::vector<InstInterval> &TraceIntervals_,
    const std::filesystem::path OFSPath_)
    : TraceIntervals(TraceIntervals_), OFSPath(OFSPath_) {

  It = TraceIntervals.begin();

  if (!isDone()) {
    OFS_ = std::ofstream(getCurOFSPath());
  }
}

bool InstTraceContext::IntervalIterator::isDone() const {
  return It == TraceIntervals.end();
}

bool InstTraceContext::IntervalIterator::advance() {
  assert(!isDone());
  ++It;
  ++Idx;
  SerializedCount = 0;

  if (!isDone()) {
    OFS_ = std::ofstream(getCurOFSPath());
    return false;
  }
  return true;
}

std::filesystem::path InstTraceContext::IntervalIterator::getCurOFSPath() {
  std::filesystem::path NewFileName = OFSPath.stem().string() + "." +
                                      std::to_string(Idx) +
                                      OFSPath.extension().string();
  return OFSPath.parent_path() / NewFileName;
}

InstTraceContext::InstTraceContext()
    : TraceIntervals(getTraceIntervals()),
      CurInterval(TraceIntervals, GetEnv::path(ENV_TRACE_PATH)) {

  for (const InstInterval &Interval : TraceIntervals) {
    std::cout << Interval << std::endl;
  }

  if (CurInterval.isDone()) {
    std::cout << "No intervals to trace" << std::endl;
    std::exit(EXIT_SUCCESS);
  }
}

void InstTraceContext::incDynamicInstCount() {
  DynInstId CurInstId = curInstId();
  TraceContext::incDynamicInstCount();

  if (CurInstId == CurInterval.instInterval().Start) {
    recordCallStack();
  }

  // Update call stack.
  assert(EnteredBB.isValid());
  if (EnteredBB.Id) {
#ifdef ENABLE_DEBUG
    std::cout << CurInstId << " Entered BB " << *EnteredBB.Id
              << " (entry: " << EnteredBB.IsFunctionEntry << ", lp: ";
    if (EnteredBB.isLandingPad()) {
      std::cout << *EnteredBB.LPFuncId;
    } else {
      std::cout << "none";
    }
    std::cout << ")" << std::endl;
    dumpCallStack();
#endif

    if (EnteredBB.isLandingPad()) {
      assert(!EnteredBB.IsFunctionEntry);
      while (!CallStack.empty() &&
             CallStack.back().FrameId != EnteredBB.LPFuncId) {
        CallStack.pop_back();
      }

      assert(!CallStack.empty());
    }

    if (EnteredBB.IsFunctionEntry) {
      CallStack.emplace_back(*EnteredBB.Id);
    } else {
      assert(!CallStack.empty());
      CallStack.back().enterBB(*EnteredBB.Id);
    }

    EnteredBB.reset();

#ifdef ENABLE_DEBUG
    std::cout << CurInstId << " Post entered BB" << std::endl;
    dumpCallStack();
#endif
  }

  assert(!CallStack.empty());
  ++CallStack.back().BBExecCount;

  // 2. Pop call stack if return is encountered.
  //    Note: this can occur if a BB only consists of a return statement.
  if (WillReturn) {
#ifdef ENABLE_DEBUG
    assert(!CallStack.empty());
    std::cout << CurInstId << " Return" << std::endl;
    dumpCallStack();
#endif

    /*ExpectNewCallFrame = CallStack.empty();*/
    CallStack.pop_back();
    WillReturn = false;

#ifdef ENABLE_DEBUG
    std::cout << CurInstId << " Post return" << std::endl;
    dumpCallStack();
#endif
  }

  // Ignore check if this instruction hasn't met the interval start.
  if (CurInterval.isWaitingForInterval(CurInstId)) {
    return;
  }

  // Check if the next instruction will be in a new interval.
  if (!CurInterval.inInterval(curInstId())) {
    if (!TEs.empty()) {
      serializeTEs();
    }

    std::cout << "Finished " << CurInterval << std::endl;
    std::cout << " - Serialize count " << CurInterval.SerializedCount
              << std::endl;

    if (CurInterval.advance()) {
      std::cout << "Finished all intervals" << std::endl;
      std::exit(EXIT_SUCCESS);
    }
  }
}

void InstTraceContext::recordFunctionEntry() {
  EnteredBB.IsFunctionEntry = true;
}

void InstTraceContext::recordReturnInst() { WillReturn = true; }

void InstTraceContext::recordBasicBlock(BBId Id) {
  EnteredBB.Id = Id;

  if (CurInterval.isWaitingForInterval(curInstId())) {
    return;
  }

  TraceEvent BBEnter;
  BBEnter.mutable_bb()->set_bb_id(Id);

  TEs.push_back(BBEnter);
  trySerializeTEs();
}

void InstTraceContext::recordLandingPad(BBId FunctionId) {
  EnteredBB.LPFuncId = FunctionId;
}

void InstTraceContext::recordLoadInst(InstId Id, void *Address) {
  if (CurInterval.isWaitingForInterval(curInstId())) {
    return;
  }

  std::uint64_t AddressU64 = reinterpret_cast<std::uint64_t>(Address);

  TraceEvent Load;
  TraceEvent::DynamicInst *Inst = Load.mutable_inst();
  Inst->set_inst_id(Id);
  Inst->mutable_memory()->set_address(AddressU64);

  TEs.push_back(Load);
  trySerializeTEs();
}

void InstTraceContext::recordStoreInst(InstId Id, void *Address) {
  if (CurInterval.isWaitingForInterval(curInstId())) {
    return;
  }

  std::uint64_t AddressU64 = reinterpret_cast<std::uint64_t>(Address);

  TraceEvent Store;
  TraceEvent::DynamicInst *Inst = Store.mutable_inst();
  Inst->set_inst_id(Id);
  Inst->mutable_memory()->set_address(AddressU64);

  TEs.push_back(Store);
  trySerializeTEs();
}
void InstTraceContext::recordCallStack() {
  dumpCallStack();

  TraceEvent PCallStack;
  TraceEvent::CallStack *PStack = PCallStack.mutable_call_stack();
  for (const CallFrame &F : CallStack) {
    TraceEvent::CallStack::CallFrame *PF = PStack->add_frames();
    PF->set_bb_id(F.CurBB);
    PF->set_bb_exec_count(F.BBExecCount);
  }
  TEs.push_back(PCallStack);
  trySerializeTEs();
}

void InstTraceContext::trySerializeTEs() {
  // assert(TEs.size() <= SerializeTESize);
  if (!CurInterval.isEndKnown() || TEs.size() == SerializeTESize) {
    serializeTEs();
  }
}

void InstTraceContext::serializeTEs() {
  assert(TEs.size() <= SerializeTESize);
  CurInterval.SerializedCount += TEs.size();
  stream::write_buffered(CurInterval.OFS(), TEs, 0);
  assert(TEs.empty());
  CurInterval.OFS().flush();
}

void InstTraceContext::dumpCallStack() const {
  std::cout << "Call Stack:" << std::endl;

  if (CallStack.empty()) {
    std::cout << " [empty]" << std::endl;
    return;
  }

  decltype(CallStack.size()) StackIdx = 0;
  for (const CallFrame &F : CallStack) {
    std::cout << "  [" << StackIdx << "] " << F << std::endl;
    ++StackIdx;
  }
}

} // namespace trace
} // namespace dragongem

void incDynamicInstCount() {
  dragongem::trace::TraceContext::getInstance()->incDynamicInstCount();
}

void recordFunctionEntry() {
  dragongem::trace::TraceContext::getInstance()->recordFunctionEntry();
}

void recordReturnInst() {
  dragongem::trace::TraceContext::getInstance()->recordReturnInst();
}

void recordBasicBlock(BBId Id) {
  dragongem::trace::TraceContext::getInstance()->recordBasicBlock(Id);
}

void recordLandingPad(BBId FunctionId) {
  dragongem::trace::TraceContext::getInstance()->recordLandingPad(FunctionId);
}

void recordLoadInst(InstId Id, void *Address) {
  dragongem::trace::TraceContext::getInstance()->recordLoadInst(Id, Address);
}

void recordStoreInst(InstId Id, void *Address) {
  dragongem::trace::TraceContext::getInstance()->recordStoreInst(Id, Address);
}

#ifdef ENABLE_DEBUG
#undef ENABLE_DEBUG
#endif
