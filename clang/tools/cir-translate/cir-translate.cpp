//===- cir-translate.cpp - CIR Translate Driver ------------------*- C++
//-*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Converts CIR directly to LLVM IR, similar to mlir-translate or LLVM llc.
//
//===----------------------------------------------------------------------===//

#include "mlir/Dialect/DLTI/DLTI.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/InitAllTranslations.h"
#include "mlir/Support/LogicalResult.h"
#include "mlir/Target/LLVMIR/Dialect/All.h"
#include "mlir/Target/LLVMIR/Import.h"
#include "mlir/Tools/mlir-translate/MlirTranslateMain.h"
#include "mlir/Tools/mlir-translate/Translation.h"

#include "llvm/IR/Module.h"
#include "llvm/TargetParser/Host.h"

#include "clang/Basic/TargetInfo.h"
#include "clang/CIR/Dialect/IR/CIRDialect.h"
#include "clang/CIR/MissingFeatures.h"

namespace cir {
namespace direct {
extern void registerCIRDialectTranslation(mlir::DialectRegistry &registry);
extern std::unique_ptr<llvm::Module> lowerDirectlyFromCIRToLLVMIR(
    mlir::ModuleOp theModule, llvm::LLVMContext &llvmCtx,
    bool disableVerifier = false, bool disableCCLowering = false);
} // namespace direct

namespace {

std::string determineDataLayoutByTriple(llvm::StringRef rawTriple) {
  llvm::Triple triple(rawTriple);
  // Data layout is fully determined by the target triple. Here we only pass the
  // triple to get the data layout.
  clang::TargetOptions targetOptions;
  targetOptions.Triple = rawTriple;
  // FIXME: AllocateTarget is a big deal. Better make it a global state.
  auto targetInfo =
      clang::targets::AllocateTarget(llvm::Triple(rawTriple), targetOptions);
  if (!targetInfo) {
    llvm::errs() << "error: invalid target triple '" << rawTriple << "'\n";
    exit(1);
  }
  return targetInfo->getDataLayoutString();
}

/// The goal of this option is to ensure that the triple and data layout specs
/// are always available in the ClangIR module. This behavior mimics the clang
/// driver, with minor tweaks.
///
/// We need to handle two scenarios:
/// 1) `cir-translate` invoked for the ClangIR pipeline.
/// For example, `clang -emit-cir | cir-translate`. In this case, we want to
/// retain the triple and data layout from the module as is.
/// 2) `cir-translate` invoked for testing or standalone purposes.
/// In this scenario, the module often does not have the triple attribute set up
/// (and we don't want to force users to set it up, as the data layout is too
/// verbose). Therefore, only when the attribute is absent, we will use the
/// `--target` option to set it up.
llvm::cl::opt<std::string>
    targetTripleOption("target",
                       llvm::cl::desc("Specify a default target triple when "
                                      "it's not available in the module"),
                       llvm::cl::init("default"));

std::string prepareCIRModuleTriple(mlir::ModuleOp mod) {
  // If the triple is already set, we don't need to do anything.
  if (auto tripleAttr = mod->getAttrOfType<mlir::StringAttr>(
          cir::CIRDialect::getTripleAttrName()))
    return tripleAttr.getValue().str();

  // Otherwise, make the `--target` option take precedence.
  std::string triple = targetTripleOption;

  // Treat "" or "default" as stand-ins for the default machine.
  if (triple.empty() || triple == "default") {
    // FIXME: ClangIR does not support OS like Windows, which may lead to
    // CI failure. Use `llvm::sys::getDefaultTargetTriple()` in the future.
    assert(!cir::MissingFeatures::supportMoreTargetTriples());
    triple = "x86_64-unknown-linux-gnu";
  }
  // Treat "native" as stand-in for the host machine.
  if (triple == "native") {
    // FIXME: ClangIR does not support OS like Windows, which may lead to
    // CI failure. Use `llvm::sys::getProcessTriple()` in the future.
    assert(!cir::MissingFeatures::supportMoreTargetTriples());
    llvm_unreachable("native target NYI");
  }

  mod->setAttr(cir::CIRDialect::getTripleAttrName(),
               mlir::StringAttr::get(mod.getContext(), triple));
  return triple;
}

void prepareCIRModuleDataLayout(mlir::ModuleOp mod, llvm::StringRef triple) {
  // If the data layout is already set, we don't need to do anything.
  if (mod->hasAttr(mlir::DLTIDialect::kDataLayoutAttrName))
    return;

  auto *context = mod.getContext();

  // Set up DLTI spec depending on the target triple.
  std::string layoutString = determineDataLayoutByTriple(triple);

  // Registered dialects may not be loaded yet, ensure they are.
  context->loadDialect<mlir::DLTIDialect, mlir::LLVM::LLVMDialect>();

  mlir::DataLayoutSpecInterface dlSpec =
      mlir::translateDataLayout(llvm::DataLayout(layoutString), context);
  mod->setAttr(mlir::DLTIDialect::kDataLayoutAttrName, dlSpec);
}

/// Prepare requirements like cir.triple and data layout.
void prepareCIRModuleForTranslation(mlir::ModuleOp mod) {
  std::string triple = prepareCIRModuleTriple(mod);
  prepareCIRModuleDataLayout(mod, triple);
}
} // namespace
} // namespace cir

void registerToLLVMTranslation() {
  static llvm::cl::opt<bool> disableCCLowering(
      "disable-cc-lowering",
      llvm::cl::desc("Disable calling convention lowering pass"),
      llvm::cl::init(false));

  mlir::TranslateFromMLIRRegistration registration(
      "cir-to-llvmir", "Translate CIR to LLVMIR",
      [](mlir::Operation *op, mlir::raw_ostream &output) {
        auto cirModule = llvm::dyn_cast<mlir::ModuleOp>(op);
        cir::prepareCIRModuleForTranslation(cirModule);
        llvm::LLVMContext llvmContext;
        auto llvmModule = cir::direct::lowerDirectlyFromCIRToLLVMIR(
            cirModule, llvmContext,
            /*disableVerifier=*/false, disableCCLowering);
        if (!llvmModule)
          return mlir::failure();
        llvmModule->print(output, nullptr);
        return mlir::success();
      },
      [](mlir::DialectRegistry &registry) {
        registry.insert<mlir::DLTIDialect, mlir::func::FuncDialect>();
        mlir::registerAllToLLVMIRTranslations(registry);
        cir::direct::registerCIRDialectTranslation(registry);
      });
}

int main(int argc, char **argv) {
  registerToLLVMTranslation();
  return failed(mlir::mlirTranslateMain(argc, argv, "CIR Translation Tool"));
}
