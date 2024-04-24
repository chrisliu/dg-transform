#ifndef DRAGONGEM_LLVM_TRACE_INSTRUMENTATION_H
#define DRAGONGEM_LLVM_TRACE_INSTRUMENTATION_H

#include <cstdint>

using InstId = std::uint64_t;
using BBId = std::uint64_t;

extern "C" {
void incDynamicInstCount();

void recordBasicBlock(BBId BBId);
void recordLoadInst(InstId Id, void *Address);
void recordStoreInst(InstId Id, void *Address);
}

#endif // DRAGONGEM_LLVM_TRACE_INSTRUMENTATION_H
