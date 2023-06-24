declare i32 @printf(i8*, ...)
declare void @exit(i32)
@.int_specifier = constant [4 x i8] c"%d\0A\00"
@.str_specifier = constant [4 x i8] c"%s\0A\00"

define void @print_0(i8*) {
    %spec_ptr = getelementptr [4 x i8], [4 x i8]* @.str_specifier, i32 0, i32 0
    call i32 (i8*, ...) @printf(i8* %spec_ptr, i8* %0)
    ret void
}

define void @printi_1(i32) {
    %spec_ptr = getelementptr [4 x i8], [4 x i8]* @.int_specifier, i32 0, i32 0
    call i32 (i8*, ...) @printf(i8* %spec_ptr, i32 %0)
    ret void
}

@.DIV_BY_ZERO_ERROR = internal constant [23 x i8] c"Error division by zero\00"
define void @check_division(i32)
{
    %valid = icmp eq i32 %0, 0
    br i1 %valid, label %div_0, label %legal_div
    div_0:
    call void @print_0(i8* getelementptr([23 x i8], [23 x i8]* @.DIV_BY_ZERO_ERROR, i32 0, i32 0))
    call void @exit(i32 0)
    ret void
    legal_div:
    ret void
}
define i32 @CosAmak_2()
{
%var_0 = alloca i32, i32 50
br label %label_4
label_4:
br label %label_8
label_6:
br label %label_8
label_8:
%var_1 = phi i32 [1, %label_4], [0, %label_6]
%var_2 = getelementptr i32, i32* %var_0, i32 0
store i32 %var_1, i32* %var_2
%var_3 = icmp eq i32 8, 8
br i1 %var_3, label %label_14, label %label_18
label_14:
%var_4 = add i32 200, 100
%var_5 = and i32 255, %var_4
ret i32 %var_5
label_18:
ret i32 2
}
define void @main()
{
%var_6 = alloca i32, i32 50
%var_7 = call i32 @CosAmak_2()
call void @printi_1(i32 %var_7)
ret void
}
