#include "InstrumentationInterface.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Module.h"

namespace dragongem {
namespace llvm {
namespace trace {

InstrumentationInterface::InstrumentationInterface(::llvm::Module &M) {
  ::llvm::LLVMContext &Ctx = M.getContext();

  ::llvm::Type *VoidTy = ::llvm::Type::getVoidTy(Ctx);
  ::llvm::Type *PtrTy = ::llvm::PointerType::get(Ctx, 0); // void *
  I64Ty = ::llvm::Type::getInt64Ty(Ctx);

  {
    std::vector<::llvm::Type *> Args;
    ::llvm::FunctionType *FTy = ::llvm::FunctionType::get(VoidTy, Args, false);
    IncDynamicInstCountFunc = M.getOrInsertFunction("incDynamicInstCount", FTy);
  }

  {
    std::vector<::llvm::Type *> Args = {I64Ty};
    ::llvm::FunctionType *FTy = ::llvm::FunctionType::get(VoidTy, Args, false);
    RecordBasicBlockFunc = M.getOrInsertFunction("recordBasicBlock", FTy);
  }

  {
    std::vector<::llvm::Type *> Args = {I64Ty, PtrTy};
    ::llvm::FunctionType *FTy = ::llvm::FunctionType::get(VoidTy, Args, false);
    RecordLoadInstFunc = M.getOrInsertFunction("recordLoadInst", FTy);
  }

  {
    std::vector<::llvm::Type *> Args = {I64Ty, PtrTy};
    ::llvm::FunctionType *FTy = ::llvm::FunctionType::get(VoidTy, Args, false);
    RecordStoreInstFunc = M.getOrInsertFunction("recordStoreInst", FTy);
  }
}

} // namespace trace
} // namespace llvm
} // namespace dragongem
