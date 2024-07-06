#include "TargetLoweringInfo.h"

namespace mlir {
namespace cir {

TargetLoweringInfo::TargetLoweringInfo(std::unique_ptr<ABIInfo> Info)
    : Info(std::move(Info)) {}

TargetLoweringInfo::~TargetLoweringInfo() = default;

AddressSpaceAttr::map_t const &TargetLoweringInfo::getCIRAddrSpaceMap() const {
  static AddressSpaceAttr::map_t defaultAddrSpaceMap = {0};
  return defaultAddrSpaceMap;
}

} // namespace cir
} // namespace mlir
