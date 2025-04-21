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

std::string MetadataFile;
static ::llvm::cl::opt<std::string, true>
    MetadataFileOpt("dg-metadata-file",
                    ::llvm::cl::desc("<output LLVM metadata file>"),
                    ::llvm::cl::location(MetadataFile));

} // namespace trace
} // namespace llvm
} // namespace dragongem
