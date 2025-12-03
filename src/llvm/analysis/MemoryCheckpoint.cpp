#include "dragongem/llvm/analysis/MemoryCheckpoint.h"

namespace dragongem {
namespace llvm {

RegionNode::RegionNode(const RegionNodeKind K) : Kind(K) {}

RegionNode::RegionNodeKind RegionNode::getKind() const { return Kind; }

} // namespace llvm
} // namespace dragongem
