; ModuleID = 'JIT'
source_filename = "JIT"

define i32 @choice(i32 %a, i32 %b, i32 %choice) {
entry:
  %addtmp = add i32 %a, %b
  %ifcond = icmp sgt i32 %addtmp, 0
  br i1 %ifcond, label %then, label %else

then:                                             ; preds = %entry
  br label %ifcont

else:                                             ; preds = %entry
  br label %ifcont

ifcont:                                           ; preds = %else, %then
  %iftmp = phi i32 [ %a, %then ], [ %b, %else ]
  ret i32 %iftmp
}
