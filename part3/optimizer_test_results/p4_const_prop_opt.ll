; ModuleID = 'opt_tests/p4_const_prop.ll'
source_filename = "p4_const_prop.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @func(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  store i32 10, ptr %3, align 4
  store i32 20, ptr %4, align 4
  store i32 20, ptr %5, align 4
  store i32 5, ptr %3, align 4
  br label %7

7:                                                ; preds = %18, %1
  %8 = load i32, ptr %3, align 4
  %9 = load i32, ptr %2, align 4
  %10 = icmp slt i32 %8, %9
  br i1 %10, label %11, label %19

11:                                               ; preds = %7
  %12 = load i32, ptr %3, align 4
  %13 = add nsw i32 %12, 1
  store i32 %13, ptr %3, align 4
  %14 = load i32, ptr %3, align 4
  %15 = icmp sgt i32 %14, 20
  br i1 %15, label %16, label %17

16:                                               ; preds = %11
  store i32 25, ptr %5, align 4
  br label %18

17:                                               ; preds = %11
  store i32 25, ptr %5, align 4
  br label %18

18:                                               ; preds = %17, %16
  br label %7, !llvm.loop !6

19:                                               ; preds = %7
  %20 = load i32, ptr %3, align 4
  call void @print(i32 noundef %20)
  call void @print(i32 noundef 20)
  %21 = load i32, ptr %5, align 4
  call void @print(i32 noundef %21)
  %22 = add nsw i32 20, %21
  ret i32 %22
}

declare void @print(i32 noundef) #1

attributes #0 = { noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"Ubuntu clang version 15.0.7"}
!6 = distinct !{!6, !7}
!7 = !{!"llvm.loop.mustprogress"}
