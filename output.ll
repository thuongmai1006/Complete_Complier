; ModuleID = 'JIT'
source_filename = "JIT"

define i32 @choice(i32 %a, i32 %b, i32 %choice) {
entry:
  br label %loop

loop:                                             ; preds = %loop, %entry
  %i = phi i32 [ 0, %entry ], [ %nextvar, %loop ]
  %addtmp = add i32 %a, %b
  %nextvar = add i32 %i, 1
  %lttmp = icmp slt i32 %i, 10
  br i1 %lttmp, label %loop, label %afterloop

afterloop:                                        ; preds = %loop
  %addtmp1 = add i32 %a, %b
  %ifcond = icmp ne i32 %addtmp1, 0
  br i1 %ifcond, label %then, label %else

then:                                             ; preds = %afterloop
  br label %ifcont

else:                                             ; preds = %afterloop
  br label %ifcont

ifcont:                                           ; preds = %else, %then
  %iftmp = phi i32 [ %a, %then ], [ %b, %else ]
  ret i32 %iftmp
}
