#include "Instrumentation.h"

#include <cassert>
#include <chrono>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
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
#include <vector>

#include "stream.hpp"
#include "trace/BBInterval.pb.h"
#include "trace/InstTrace.pb.h"

#define ENABLE_ASSERT
// #define ENABLE_DEBUG

#ifdef ENABLE_ASSERT
#define ASSERT(expr) assert(expr)
#else
#define ASSERT(expr)
#endif

namespace {

using PBBFrame = dragongem::trace::BBFrame;
using PTraceEvent = dragongem::trace::TraceEvent;

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
const InstId InvalidInst = 0; // dragongem::llvm::CanonicalId::InvalidInstId;
const BBId InvalidBB = 0;     // dragongem::llvm::CanonicalId::InvalidBBId;

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
//   record...() [any order]
//   incDynamicInstCount() [always last]
//   execute actual instruction
class TraceContext {
public:
  static TraceContext *getInstance();

  virtual void incDynamicInstCount() { ++CurInstId; }

  virtual CallId getCallSite(const InstId Id) { return InvalidCall; }
  virtual void recordReturnFromCall(const CallId Id,
                                    const InstId NumRetiredInBB) {}

  virtual void recordBasicBlock(const BBId Id, const bool IsFuncEntry) {}

  virtual void recordLoadInst(const InstId Id, void *const Address) {}
  virtual void recordStoreInst(const InstId Id, void *const Address) {}

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
  void recordBasicBlock(const BBId Id, const bool IsFuncEntry) override;

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

    std::chrono::steady_clock::time_point TimeFF;
    std::chrono::steady_clock::time_point TimeStart;

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
    friend std::ostream &operator<<(std::ostream &OS, const CallFrame &F) {
      OS << "Frame " << F.CurBB << "::" << F.NumRetired;
      if (F.PendingCall) {
        OS << " (@" << F.PendingCall.Handle << ", " << F.PendingCall.Id
           << ", isReal: " << F.PendingCall.IsReal << ")";
      }
      return OS;
    }

    BBId CurBB;
    InstId NumRetired = 0;

    struct {
      explicit operator bool() const {
        ASSERT((Id != InvalidInst) == (Handle != InvalidCall));
        return Id != InvalidInst && Handle != InvalidCall;
      }
      void reset() {
        Id = InvalidInst;
        Handle = InvalidCall;
        IsReal = false;
      }

      InstId Id = InvalidInst;
      CallId Handle = InvalidCall;
      bool IsReal = false; // Is a real call instruction.
    } PendingCall;

    CallFrame(const BBId Id) : CurBB(Id) {}
  };

public:
  InstTraceContext();

  void incDynamicInstCount() override;
  CallId getCallSite(const InstId Id) override;
  void recordReturnFromCall(const CallId Id,
                            const InstId NumRetiredInBB) override;
  void recordBasicBlock(const BBId Id, const bool IsFuncEntry) override;
  void recordLoadInst(const InstId Id, void *const Address) override;
  void recordStoreInst(const InstId Id, void *const Address) override;

protected:
  const DynInstId SerializeTESize = 1000; // Serialize every X TEs.
  // const DynInstId SerializeTESize = 1; // Serialize every X TEs.
  CallId CurCallHandle = InvalidCall + 1;
  std::chrono::steady_clock::time_point TimeAllStart;

  const std::vector<InstInterval> TraceIntervals;
  IntervalIterator CurInterval;

  std::deque<CallFrame> CallStack;

  struct {
    void reset() {
      EnteredBB.reset();
      Return.reset();
      Call.reset();
      Memory.reset();
    }

    std::string debugStr() const {
      std::stringstream SS;
      if (Return) {
        SS << "Return(@" << Return.Handle << "::" << Return.NumRetired << ") ";
      }
      if (EnteredBB) {
        SS << "EnteredBB(" << EnteredBB.Id
           << ", isFuncEntry: " << EnteredBB.IsFuncEntry << ") ";
      }
      if (Call) {
        SS << "Call(@" << Call.Handle << ", " << Call.Id << ") ";
      }
      if (Memory) {
        SS << "Memory(" << Memory.IsLoad << "|" << Memory.IsStore << ", "
           << Memory.Id << ", " << Memory.Address << ") ";
      }
      return SS.str();
    }

    struct {
      explicit operator bool() const { return Id != InvalidBB; }
      void reset() {
        Id = InvalidBB;
        IsFuncEntry = false;
      }

      BBId Id = InvalidBB;
      bool IsFuncEntry = false;
    } EnteredBB;

    struct {
      explicit operator bool() const { return Handle != InvalidCall; }
      void reset() {
        Handle = InvalidCall;
        NumRetired = 0;
      }

      CallId Handle = InvalidCall;
      InstId NumRetired = 0;
    } Return;

    struct {
      explicit operator bool() const {
        ASSERT((Id != InvalidInst) == (Handle != InvalidCall));
        return Id != InvalidInst && Handle != InvalidCall;
      }
      void reset() {
        Id = InvalidInst;
        Handle = InvalidCall;
      }

      InstId Id = InvalidInst;
      CallId Handle = InvalidCall;
    } Call;

    struct {
      explicit operator bool() const {
        ASSERT((Id != InvalidInst) == (Address != nullptr));
        ASSERT(!(IsLoad && IsStore));
        ASSERT((IsLoad || IsStore) == (Id != InvalidInst));
        return (IsLoad || IsStore) && Id != InvalidInst && Address != nullptr;
      }
      void reset() {
        IsLoad = false;
        IsStore = false;
        Id = InvalidInst;
        Address = nullptr;
      }

      bool IsLoad = false;
      bool IsStore = false;
      InstId Id = InvalidInst;
      void *Address = nullptr;
    } Memory;
  } CurTick;

  std::vector<dragongem::trace::TraceEvent> TEs;

  void setSerialize(const bool CanSerialize_);

  void serializeCallStack();
  void serializeStackAdjust(const BBId TopBB, const InstId TopNumRetired,
                            const std::uint64_t NumPoppedFrames);
  void serializeStackAdjust(const BBId TopBB, const InstId TopNumRetired,
                            const std::uint64_t NumPoppedFrames,
                            const BBId NewBB, const InstId NewNumRetired);
  void serializeCall(const InstId Id);
  void serializeBBEnter(const BBId Id);
  void serializeMemory(const InstId Id, void *const Address);

  void trySerializeTEs();
  void serializeTEs();

  void dumpCallStack() const;

private:
  bool CanSerialize = false;
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
    dragongem::trace::BBInterval BBI;
    BBI.set_inst_start(curInstId() - IntervalSize);
    BBI.set_inst_end(curInstId() - 1);
    BBI.mutable_freq()->insert(BBVec.begin(), BBVec.end());

    std::function<dragongem::trace::BBInterval(uint64_t)> EmitBBI =
        [&BBI](uint64_t) { return BBI; };
    stream::write(BBIntervalOFS, 1, EmitBBI);
    BBIntervalOFS.flush();

    BBVec.clear();
  }
}

void SimPointContext::recordBasicBlock(BBId Id, const bool IsFuncEntry) {
  ++BBVec[Id];
}

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

  std::cout << "Assigned Intervals:" << std::endl;
  for (auto i = 0; i < TraceIntervals.size(); ++i) {
    std::cout << "  " << (i + 1) << ". " << TraceIntervals[i] << std::endl;
  }

  if (CurInterval.isDone()) {
    std::cout << "No intervals to trace" << std::endl;
    std::exit(EXIT_SUCCESS);
  }

  const auto Now = std::chrono::steady_clock::now();
  TimeAllStart = Now;
  CurInterval.TimeFF = Now;
}

void InstTraceContext::incDynamicInstCount() {
  const DynInstId CurInstId = curInstId();
  TraceContext::incDynamicInstCount();
  const InstId NextInstId = curInstId();

#ifdef ENABLE_DEBUG
  // 807'119'079
  // const bool DebugOn = CurInstId >= 807'119'000;
  const bool DebugOn = true;
#endif

#ifdef ENABLE_DEBUG
  if (DebugOn) {
    std::cout << std::endl;
    std::cout << "Inst " << CurInstId << " = " << CurTick.debugStr()
              << std::endl;
  }
#endif

  const bool IsFirstInInterval = CurInstId == CurInterval.instInterval().Start;

#ifdef ENABLE_DEBUG
  const bool WillStackUpdate =
      CurTick.Return || CurTick.EnteredBB || CurTick.Call;
  if (DebugOn) {
    if (WillStackUpdate) {
      dumpCallStack();
    }
  }
#endif

  // Resolve the effects of any branch/return/exception handling instruction
  // that executed *BEFORE* this instruction.
  // 1. Resolve any returns first (either plain return or exception unwinding).
  // 2. Resolve any basic block entry (either new call, branch,
  //    normal/landingpad).
  bool IgnoreBBEnter = false;
  if (CurTick.Return) {
    ASSERT(!CallStack.empty());

    CallFrame &CalleeFrame = CallStack.back();
    if (CalleeFrame.PendingCall.Handle == CurTick.Return.Handle) {
      // Case: Called function was not traced.
      ASSERT(!CalleeFrame.PendingCall.IsReal);
      ASSERT(CurTick.EnteredBB ||
             CalleeFrame.NumRetired == CurTick.Return.NumRetired);

      CalleeFrame.PendingCall.reset();
    } else {
      BBId CalleeBB = CalleeFrame.CurBB;
      InstId CalleeNumRetired = CalleeFrame.NumRetired;

      assert(CurTick.Return.Handle != InvalidCall);

      std::uint64_t NumPopped = 0;
      while (!CallStack.empty() &&
             CallStack.back().PendingCall.Handle != CurTick.Return.Handle) {
        ++NumPopped;
        CallStack.pop_back();
      }
      // Note: CalleeFrame invalid here.

      if (CallStack.empty()) {
        std::cout << CurInstId << std::endl;
      }
      assert(!CallStack.empty());

      CallFrame &CurFrame = CallStack.back();

      ASSERT(CurFrame.PendingCall.IsReal);
      CurFrame.PendingCall.reset();

      CurFrame.NumRetired = CurTick.Return.NumRetired;
      if (CurTick.EnteredBB) {
        ASSERT(!CurTick.EnteredBB.IsFuncEntry);
        IgnoreBBEnter = true;
        CurFrame.CurBB = CurTick.EnteredBB.Id;
        serializeStackAdjust(CalleeBB, CalleeNumRetired, NumPopped,
                             CurFrame.CurBB, CurFrame.NumRetired);
      } else {
        serializeStackAdjust(CalleeBB, CalleeNumRetired, NumPopped);
      }
    }
  }

  if (!IgnoreBBEnter && CurTick.EnteredBB) {
    if (CurTick.EnteredBB.IsFuncEntry) {
      if (!CallStack.empty()) {
        if (CallStack.back().PendingCall) {
          CallStack.back().PendingCall.IsReal = true;

          serializeCall(CallStack.back().PendingCall.Id);
        } else {
          // Special case: sometimes we're supposed to exit a global variable
          //               constructor and into the actual main function.
          //               https://maskray.me/blog/2021-11-07-init-ctors-init-array
          ASSERT(CallStack.size() == 1);
          CallFrame &CurFrame = CallStack.back();

          serializeStackAdjust(CurFrame.CurBB, CurFrame.NumRetired, 1);

          CallStack.pop_back();
        }
      }

      CallStack.emplace_back(CurTick.EnteredBB.Id);
    } else {
      assert(!CallStack.empty());
      ASSERT(!CallStack.back().PendingCall);

      CallStack.back().CurBB = CurTick.EnteredBB.Id;
      CallStack.back().NumRetired = 0;
    }

    serializeBBEnter(CurTick.EnteredBB.Id);
  }

  // If we entered a new interval, save the call stack *BEFORE* executing this
  // instruction.
  if (IsFirstInInterval) {
    const auto Now = std::chrono::steady_clock::now();
    CurInterval.TimeStart = Now;

    const auto TotElapsed =
        std::chrono::duration_cast<std::chrono::seconds>(Now - TimeAllStart);
    const auto FFElapsed = std::chrono::duration_cast<std::chrono::seconds>(
        Now - CurInterval.TimeFF);

    std::cout << CurInterval << std::endl;
    std::cout << "[FF Time]    " << FFElapsed.count() << " s" << std::endl
              << "[Total Time] " << TotElapsed.count() << " s " << std::endl;
    dumpCallStack();

    setSerialize(true); // Enable serialization for this interval here.
    serializeCallStack();
  }

  // Update basic block with information.
  assert(!CallStack.empty());
  CallFrame &CurFrame = CallStack.back();
  ++CurFrame.NumRetired;

  if (CurTick.Call) {
    ASSERT(!CurFrame.PendingCall);
    CurFrame.PendingCall.Id = CurTick.Call.Id;
    CurFrame.PendingCall.Handle = CurTick.Call.Handle;
  }

  if (CurTick.Memory) {
    serializeMemory(CurTick.Memory.Id, CurTick.Memory.Address);
  }

  CurTick.reset();

  if (!CurInterval.inInterval(CurInstId)) {
    return;
  }

#ifdef ENABLE_DEBUG
  if (DebugOn) {
    if (WillStackUpdate) {
      dumpCallStack();
    }
  }
#endif

  // Check if the next instruction belongs to the current interval.
  if (!CurInterval.inInterval(NextInstId)) {
    if (!TEs.empty()) {
      serializeTEs();
    }

    setSerialize(false);

    const auto Now = std::chrono::steady_clock::now();
    const auto TraceElapsed = std::chrono::duration_cast<std::chrono::seconds>(
        Now - CurInterval.TimeStart);

    std::cout << "Finished " << CurInterval << std::endl;
    std::cout << " - Serialize count " << CurInterval.SerializedCount
              << std::endl;
    std::cout << "[Trace Time] " << TraceElapsed.count() << " s" << std::endl;

    if (CurInterval.advance()) {
      std::cout << "Finished all intervals" << std::endl;
      std::exit(EXIT_SUCCESS);
    } else {
      CurInterval.TimeFF = Now;
    }
  }
}

CallId InstTraceContext::getCallSite(const InstId Id) {
  CallId Handle = CurCallHandle;
  ++CurCallHandle;

  CurTick.Call.Id = Id;
  CurTick.Call.Handle = Handle;

  return Handle;
}

void InstTraceContext::recordReturnFromCall(const CallId Id,
                                            const InstId NumRetiredInBB) {
  if (Id != InvalidCall) {
    CurTick.Return.Handle = Id;
    CurTick.Return.NumRetired = NumRetiredInBB;
  }
}

void InstTraceContext::recordBasicBlock(const BBId Id, const bool IsFuncEntry) {
  CurTick.EnteredBB.Id = Id;
  CurTick.EnteredBB.IsFuncEntry = IsFuncEntry;
}

void InstTraceContext::recordLoadInst(const InstId Id, void *const Address) {
  CurTick.Memory.IsLoad = true;
  CurTick.Memory.Id = Id;
  CurTick.Memory.Address = Address;
}

void InstTraceContext::recordStoreInst(const InstId Id, void *const Address) {
  CurTick.Memory.IsStore = true;
  CurTick.Memory.Id = Id;
  CurTick.Memory.Address = Address;
}

void InstTraceContext::setSerialize(const bool CanSerialize_) {
  CanSerialize = CanSerialize_;
}

void InstTraceContext::serializeCallStack() {
  assert(CanSerialize);

  PTraceEvent TE;
  PTraceEvent::CallStack *PCallStack = TE.mutable_call_stack();
  for (const CallFrame &F : CallStack) {
    PBBFrame *PF = PCallStack->add_frames();
    PF->set_bb_id(F.CurBB);
    PF->set_num_retired(F.NumRetired);
    PF->set_is_call(F.PendingCall && F.PendingCall.IsReal);
  }

  TEs.push_back(TE);
  trySerializeTEs();
}

void InstTraceContext::serializeStackAdjust(
    const BBId TopBB, const InstId TopNumRetired,
    const std::uint64_t NumPoppedFrames) {
  if (!CanSerialize) {
    return;
  }

  PTraceEvent TE;
  PTraceEvent::StackAdjustment *StackAdj = TE.mutable_stack_adjustment();

  PBBFrame *Top = StackAdj->mutable_top_frame();
  Top->set_bb_id(TopBB);
  Top->set_num_retired(TopNumRetired);

  StackAdj->set_num_popped_frames(NumPoppedFrames);

  TEs.push_back(TE);
  trySerializeTEs();
}

void InstTraceContext::serializeStackAdjust(const BBId TopBB,
                                            const InstId TopNumRetired,
                                            const std::uint64_t NumPoppedFrames,
                                            const BBId NewBB,
                                            const InstId NewNumRetired) {
  if (!CanSerialize) {
    return;
  }

  PTraceEvent TE;
  PTraceEvent::StackAdjustment *StackAdj = TE.mutable_stack_adjustment();

  PBBFrame *Top = StackAdj->mutable_top_frame();
  Top->set_bb_id(TopBB);
  Top->set_num_retired(TopNumRetired);

  StackAdj->set_num_popped_frames(NumPoppedFrames);

  PBBFrame *New = StackAdj->mutable_new_frame();
  New->set_bb_id(NewBB);
  New->set_num_retired(NewNumRetired);

  TEs.push_back(TE);
  trySerializeTEs();
}

void InstTraceContext::serializeCall(const InstId Id) {
  if (!CanSerialize) {
    return;
  }

  PTraceEvent TE;
  PTraceEvent::DynamicInst *DynInst = TE.mutable_inst();
  DynInst->set_inst_id(Id);
  DynInst->mutable_call();

  TEs.push_back(TE);
  trySerializeTEs();
}

void InstTraceContext::serializeBBEnter(const BBId Id) {
  if (!CanSerialize) {
    return;
  }

  PTraceEvent TE;
  PTraceEvent::BBEnter *BBEnter = TE.mutable_bb();
  BBEnter->set_bb_id(Id);

  TEs.push_back(TE);
  trySerializeTEs();
}

void InstTraceContext::serializeMemory(const InstId Id, void *const Address) {
  if (!CanSerialize) {
    return;
  }

  std::uint64_t AddressU64 = reinterpret_cast<std::uint64_t>(Address);

  PTraceEvent TE;
  PTraceEvent::DynamicInst *DynInst = TE.mutable_inst();
  DynInst->set_inst_id(Id);
  DynInst->mutable_memory()->set_address(AddressU64);

  TEs.push_back(TE);
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

  // #ifdef ENABLE_DEBUG
  //   std::cout << "Serialize Protobuf" << std::endl;
  //   for (auto i = 0; i < TEs.size(); ++i) {
  //     std::cout << "  [" << i << "] " << TEs[i].ShortDebugString() <<
  //     std::endl;
  //   }
  // #endif

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

} // namespace

void incDynamicInstCount() {
  TraceContext::getInstance()->incDynamicInstCount();
}

CallId getCallSite(const InstId Id) {
  return TraceContext::getInstance()->getCallSite(Id);
}

void recordReturnFromCall(const CallId Id, const InstId NumRetiredInBB) {
  TraceContext::getInstance()->recordReturnFromCall(Id, NumRetiredInBB);
}

void recordBasicBlock(const BBId Id, const bool IsFuncEntry) {
  TraceContext::getInstance()->recordBasicBlock(Id, IsFuncEntry);
}

void recordLoadInst(const InstId Id, void *const Address) {
  TraceContext::getInstance()->recordLoadInst(Id, Address);
}

void recordStoreInst(const InstId Id, void *const Address) {
  TraceContext::getInstance()->recordStoreInst(Id, Address);
}

#ifdef ENABLE_DEBUG
#undef ENABLE_DEBUG
#endif
