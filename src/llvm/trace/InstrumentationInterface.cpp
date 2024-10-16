#include "InstrumentationInterface.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Module.h"

namespace dragongem {
namespace llvm {
namespace trace {

InstrumentationInterface::InstrumentationInterface(::llvm::Module &M) {
  ::llvm::LLVMContext &Ctx = M.getContext();

  ::llvm::Type *VoidTy = ::llvm::Type::getVoidTy(Ctx);
  ::llvm::Type *PtrTy = ::llvm::PointerType::get(Ctx, 0); // void *
                                                          //
  I64Ty = ::llvm::Type::getInt64Ty(Ctx);

  BoolTy = ::llvm::Type::getInt8Ty(Ctx);
  TrueVal = ::llvm::ConstantInt::get(BoolTy, 1);
  FalseVal = ::llvm::ConstantInt::get(BoolTy, 0);

  {
    std::vector<::llvm::Type *> Args;
    ::llvm::FunctionType *FTy = ::llvm::FunctionType::get(VoidTy, Args, false);
    IncDynamicInstCountFunc = M.getOrInsertFunction("incDynamicInstCount", FTy);
  }

  {
    std::vector<::llvm::Type *> Args = {I64Ty};
    ::llvm::FunctionType *FTy = ::llvm::FunctionType::get(I64Ty, Args, false);
    GetCallSiteFunc = M.getOrInsertFunction("getCallSite", FTy);
  }

  {
    std::vector<::llvm::Type *> Args = {I64Ty, I64Ty};
    ::llvm::FunctionType *FTy = ::llvm::FunctionType::get(VoidTy, Args, false);
    RecordReturnFromCallFunc =
        M.getOrInsertFunction("recordReturnFromCall", FTy);
  }

  {
    std::vector<::llvm::Type *> Args = {I64Ty, BoolTy};
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

  {
    std::vector<::llvm::Type *> Args = {I64Ty, PtrTy};
    ::llvm::FunctionType *FTy = ::llvm::FunctionType::get(VoidTy, Args, false);
    RecordStoreInstFunc = M.getOrInsertFunction("recordStoreInst", FTy);
  }
}

} // namespace trace
} // namespace llvm
} // namespace dragongem
