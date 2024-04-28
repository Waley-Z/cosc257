; ModuleID = 'p3_const_prop.c'
source_filename = "p3_const_prop.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @func(i32 noundef %0) #0 {
  %2 = alloca i32, align 4  // int p
  %3 = alloca i32, align 4  // int a
  %4 = alloca i32, align 4  // int b
  %5 = alloca i32, align 4  // int c
  store i32 %0, ptr %2, align 4  // p
  store i32 10, ptr %3, align 4  // a = 10
  store i32 20, ptr %4, align 4  // b = 20
  %6 = load i32, ptr %3, align 4  // load a
  %7 = load i32, ptr %4, align 4  // load b
  %8 = add nsw i32 %6, %7  // a + b
  store i32 %8, ptr %5, align 4  // c = a + b
  %9 = load i32, ptr %3, align 4  // load a
  %10 = load i32, ptr %2, align 4  // load p
  %11 = icmp slt i32 %9, %10  // a < p
  br i1 %11, label %12, label %13

12:                                               ; preds = %1
  store i32 30, ptr %4, align 4  // b = 30
  br label %15

13:                                               ; preds = %1
  %14 = load i32, ptr %5, align 4  // load c
  store i32 %14, ptr %4, align 4  // b = c
  br label %15

15:                                               ; preds = %13, %12
  %16 = load i32, ptr %4, align 4  // load b
  %17 = load i32, ptr %3, align 4  // load a
  %18 = add nsw i32 %16, %17  // b + a
  ret i32 %18
}

attributes #0 = { noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"Ubuntu clang version 15.0.7"}
