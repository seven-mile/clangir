#include "TargetLoweringInfo.h"

namespace mlir {
namespace cir {

TargetLoweringInfo::TargetLoweringInfo(std::unique_ptr<ABIInfo> Info)
    : Info(std::move(Info)) {}

TargetLoweringInfo::~TargetLoweringInfo() = default;

AddressSpaceAttr::MapTy const &TargetLoweringInfo::getCIRAddrSpaceMap() const {
  static AddressSpaceAttr::MapTy defaultAddrSpaceMap = {0};
  return defaultAddrSpaceMap;
}

} // namespace cir
} // namespace mlir
