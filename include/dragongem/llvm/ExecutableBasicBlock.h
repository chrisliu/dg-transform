#ifndef LLVM_EXECUTBALE_BASIC_BLOCK_H
#define LLVM_EXECUTBALE_BASIC_BLOCK_H

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include <vector>

namespace dragongem {
namespace llvm {

// TODO: convert this to a class with an iterator.
using ExecutableBasicBlock = std::vector<::llvm::Instruction *>;
using ExecutableConstBasicBlock = std::vector<const ::llvm::Instruction *>;

ExecutableBasicBlock getExecutableBasicBlock(::llvm::BasicBlock &BB);
ExecutableConstBasicBlock getExecutableBasicBlock(const ::llvm::BasicBlock &BB);

bool isExecInst(const ::llvm::Instruction &I);

} // namespace llvm
} // namespace dragongem

#endif // LLVM_EXECUTBALE_BASIC_BLOCK_H
