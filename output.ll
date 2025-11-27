; ModuleID = 'JIT'
source_filename = "JIT"

define i32 @calculate(i32 %0, i32 %1) {
entry:
  %addtmp = add i32 %0, %1
  ret i32 %addtmp
}
