// RUN: %clang_cc1 -cl-std=CL3.0 -O0 -fclangir -emit-cir -triple spirv64-unknown-unknown %s -o %t.cir
// RUN: FileCheck --input-file=%t.cir %s --check-prefix=CIR
// RUN: %clang_cc1 -cl-std=CL3.0 -O0 -fclangir -S -emit-llvm -triple spirv64-unknown-unknown %s -o %t.ll
// RUN: FileCheck --input-file=%t.ll %s --check-prefix=LLVM

// CIR: cir.func @foo(%arg0: !cir.ptr<!s32i, addrspace(3)>
// LLVM: @foo(ptr addrspace(3)
kernel void foo(local int *p) {
  // CIR-NEXT: %[[#ALLOCA_P:]] = cir.alloca !cir.ptr<!s32i, addrspace(3)>, !cir.ptr<!cir.ptr<!s32i, addrspace(3)>>, ["p", init] {alignment = 8 : i64}
  // LLVM-NEXT: %[[#ALLOCA_P:]] = alloca ptr addrspace(3), i64 1, align 8

  int x;
  // CIR-NEXT: %[[#ALLOCA_X:]] = cir.alloca !s32i, !cir.ptr<!s32i>, ["x"] {alignment = 4 : i64}
  // LLVM-NEXT: %[[#ALLOCA_X:]] = alloca i32, i64 1, align 4

  global char *b;
  // CIR-NEXT: %[[#ALLOCA_B:]] = cir.alloca !cir.ptr<!s8i, addrspace(1)>, !cir.ptr<!cir.ptr<!s8i, addrspace(1)>>, ["b"] {alignment = 8 : i64}
  // LLVM-NEXT: %[[#ALLOCA_B:]] = alloca ptr addrspace(1), i64 1, align 8

  // Store of the argument `p`
  // CIR-NEXT: cir.store %arg0, %[[#ALLOCA_P]] : !cir.ptr<!s32i, addrspace(3)>, !cir.ptr<!cir.ptr<!s32i, addrspace(3)>>
  // LLVM-NEXT: store ptr addrspace(3) %{{[0-9]+}}, ptr %[[#ALLOCA_P]], align 8

  return;
}

// CIR: cir.func @bar(
// LLVM: @bar(
kernel void bar() {
  void * arr = __builtin_alloca(2 * sizeof(int));
  // CIR:      %[[#ALLOCA:]] = cir.alloca !u8i, !cir.ptr<!u8i>, %{{[0-9]+}} : !u64i, ["bi_alloca"] {alignment = 8 : i64}
  // CIR-NEXT: %[[#PRIVATE_VOID_PTR:]] = cir.cast(bitcast, %[[#ALLOCA]] : !cir.ptr<!u8i>), !cir.ptr<!void>
  // CIR-NEXT: %[[#GENERIC_VOID_PTR:]] = cir.cast(address_space, %[[#PRIVATE_VOID_PTR:]] : !cir.ptr<!void>), !cir.ptr<!void, addrspace(4)>
  // CIR-NEXT: cir.store %[[#GENERIC_VOID_PTR]], %{{[0-9]+}} : !cir.ptr<!void, addrspace(4)>, !cir.ptr<!cir.ptr<!void, addrspace(4)>>

  // LLVM:      %[[#ALLOCA:]] = alloca i8, i64 8, align 8
  // LLVM-NEXT: %[[#GENERIC_VOID_PTR:]] = addrspacecast ptr %[[#ALLOCA]] to ptr addrspace(4)
  // LLVM-NEXT: store ptr addrspace(4) %[[#GENERIC_VOID_PTR]], ptr %{{[0-9]+}}, align 8

}
