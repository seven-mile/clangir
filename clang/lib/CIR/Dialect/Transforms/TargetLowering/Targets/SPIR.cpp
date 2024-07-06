//===- SPIR.cpp - TargetInfo for SPIR and SPIR-V --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ABIInfoImpl.h"
#include "LowerFunctionInfo.h"
#include "LowerTypes.h"
#include "TargetInfo.h"
#include "TargetLoweringInfo.h"
#include "clang/CIR/ABIArgInfo.h"
#include "clang/CIR/Dialect/IR/CIRTypes.h"
#include "clang/CIR/MissingFeatures.h"
#include "llvm/Support/ErrorHandling.h"

using ABIArgInfo = ::cir::ABIArgInfo;
using MissingFeature = ::cir::MissingFeatures;

namespace mlir {
namespace cir {

//===----------------------------------------------------------------------===//
// SPIR-V ABI Implementation
//===----------------------------------------------------------------------===//

namespace {

class SPIRVABIInfo : public ABIInfo {
public:
  SPIRVABIInfo(LowerTypes &CGT) : ABIInfo(CGT) {}

private:
  void computeInfo(LowerFunctionInfo &FI) const override {
    llvm_unreachable("ABI NYI");
  }
};

//===----------------------------------------------------------------------===//
// SPIR-V other target-specific information
//===----------------------------------------------------------------------===//

static AddressSpaceAttr::map_t SPIRVAddrSpaceMap = {
    0, // None
    0, // offload_private
    3, // offload_local
    1, // offload_global
    2, // offload_constant
    4, // offload_generic
};

class SPIRVTargetLoweringInfo : public TargetLoweringInfo {
public:
  SPIRVTargetLoweringInfo(LowerTypes &LT)
      : TargetLoweringInfo(std::make_unique<SPIRVABIInfo>(LT)) {}

  AddressSpaceAttr::map_t const &getCIRAddrSpaceMap() const override {
    return SPIRVAddrSpaceMap;
  }
};

} // namespace

std::unique_ptr<TargetLoweringInfo>
createSPIRVTargetLoweringInfo(LowerModule &CGM) {
  return std::make_unique<SPIRVTargetLoweringInfo>(CGM.getTypes());
}

} // namespace cir
} // namespace mlir
