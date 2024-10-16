#ifndef DRAGONGEM_LLVM_TRACE_INSTRUMENTATION_H
#define DRAGONGEM_LLVM_TRACE_INSTRUMENTATION_H

#include <cstdint>

using InstId = std::uint64_t;
using BBId = std::uint64_t;
using CallId = std::uint64_t;

extern "C" {

const CallId InvalidCall = 0;

void incDynamicInstCount();

// Get a handler for a call/invoke instruction. This will be passed to
// recordReturnFromCall when the call completes (or invoke gets handled).
CallId getCallSite(const InstId Id);
// Record returning/handle landingpad to a corresponding call/invoke inst
// handler. Also pass the number of instructions that have been retired in
// the current BB (to handle returning in the middle of a BB).
void recordReturnFromCall(const CallId Id, const InstId NumRetiredInBB);

void recordBasicBlock(const BBId Id, const bool IsFuncEntry);

void recordLoadInst(const InstId Id, void *const Address);
void recordStoreInst(const InstId Id, void *const Address);
}

#endif // DRAGONGEM_LLVM_TRACE_INSTRUMENTATION_H
