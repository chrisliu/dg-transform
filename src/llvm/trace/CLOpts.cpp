#include "CLOpts.h"
#include "llvm/Support/CommandLine.h"
#include <string>

namespace dragongem {
namespace llvm {
namespace trace {

std::string UIDFile;
static ::llvm::cl::opt<std::string, true>
    UIDFileOpt("dg-llvm-uid-file",
               ::llvm::cl::desc("<input/output LLVM UID file>"),
               ::llvm::cl::location(UIDFile));

} // namespace trace
} // namespace llvm
} // namespace dragongem
