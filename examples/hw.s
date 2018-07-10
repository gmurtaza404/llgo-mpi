; ModuleID = 'main'
source_filename = "main"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@"main..import$descriptor" = constant i8* bitcast (void (i8*)* @main..import to i8*)
@"init$guard" = internal global i1 false
@"main.main$descriptor" = constant i8* bitcast (void (i8*)* @main.main to i8*)

declare void @__go_go(i8* nest, i8*, i8*)

define void @main..import(i8* nest) #0 {
prologue:
  %1 = load i1, i1* @"init$guard"
  br i1 %1, label %2, label %3

.0.entry:                                         ; preds = %3
  ret void

; <label>:2:                                      ; preds = %prologue
  ret void

; <label>:3:                                      ; preds = %prologue
  store i1 true, i1* @"init$guard"
  br label %.0.entry
}

define void @main.main(i8* nest) #0 {
prologue:
  br label %.0.entry

.0.entry:                                         ; preds = %prologue
  call void @__go_go(i8* nest undef, i8* bitcast (void (i8*)* @0 to i8*), i8* null)
  ret void
}

define internal void @0(i8*) #0 {
prologue:
  br label %entry

entry:                                            ; preds = %prologue
  call void @"main.main:main.main$1"(i8* nest undef)
  ret void
}

define internal void @"main.main:main.main$1"(i8* nest) #0 {
prologue:
  br label %.0.entry

.0.entry:                                         ; preds = %prologue
  ret void
}

define void @__go_init_main(i8*) #0 {
entry:
  call void @main..import(i8* undef)
  ret void
}

attributes #0 = { "disable-tail-calls"="true" "split-stack" }
