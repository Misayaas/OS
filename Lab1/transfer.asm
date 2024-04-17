section .data
    InputInformation db "Input:", 0 ;0为字符串结束符
    OutputInformation db "Output:", 0
    InvalidInput db "Error", 0

    ;LenInput equ $-InputInformation
    ;LenOutput equ $-OutputInformation
    ;LenInvalidInput equ $-InvalidInput

    NewLine db 0xA ;0xA为换行符

section .bss
    inputBuffer resb 40 ;用于处理大整数
    IntegerBuffer resb 40 ;用于处理大整数
    resultBuffer resb 128 ;存储结果

section .text
    global _start

_start:
    xor eax, eax
    xor ebx, ebx
    xor ecx, ecx
    xor edx, edx

clean:
    mov ecx, 40 ; inputBuffer的大小
    mov edi, inputBuffer ; inputBuffer的地址
    cld ; 清除方向标志，使得edi递增
    rep stosb ; 使用al（这里为0）填充es:[edi]，然后根据方向标志增减edi，重复ecx次

    mov ecx, 40
    mov edi, IntegerBuffer
    cld
    rep stosb

    mov ecx, 128
    mov edi, resultBuffer
    cld
    rep stosb

    ; 系统调用输入提示
    ;mov eax, 4
    ;mov ebx, 1
    ;mov ecx, InputInformation
    ;mov edx, 6
    ;int 0x80

get_input:
    mov esi, inputBuffer
    dec esi
read_loop:
    ; 读取输入
    inc esi
    mov eax, 3
    mov ebx, 0
    mov ecx, esi
    mov edx, 1
    int 0x80
    cmp byte[esi], 0xA ; 检查是否换行
    jnz read_loop

    ; 检查是否退出程序
    cmp byte[inputBuffer], 'q'
    je exit

    mov esi, inputBuffer ;esi是源地址寄存器
    call judge_input ;调用判断函数

    test eax, eax ;AND运算测试edx的值是否为0
    jz invalid_input ;如果为0则跳转到invalid_input

    ;八进制和二进制
    cmp r10, 8
    je octal_converse
    cmp r10, 2
    je binary_converse

    cmp r10, 16
    je hexa_converse

binary_converse:
    call convert_number
    jmp outputLabel

octal_converse:
    call convert_number
    jmp outputLabel


hexa_converse:
    call convert_number
    jmp outputLabel

outputLabel:

    ; 输出提示信息
    ;mov eax, 4
    ;mov ebx, 1
    ;mov ecx, OutputInformation
    ;mov edx, 7
    ;int 0x80

    ; 输出结果
    mov esi, resultBuffer
    call strlen
    mov edx, eax;这里到时候要填result的长度
    mov eax, 4
    mov ebx, 1
    mov ecx, resultBuffer

    int 0x80

    ; 输出换行符
    mov eax, 4
    mov ebx, 1
    mov ecx, NewLine
    mov edx, 1
    int 0x80

    jmp _start

invalid_input:
    ; 输出错误信息
    mov eax, 4
    mov ebx, 1
    mov ecx, InvalidInput
    mov edx, 5
    int 0x80

    ; 输出换行符
    mov eax, 4
    mov ebx, 1
    mov ecx, NewLine
    mov edx, 1
    int 0x80

    jmp _start

exit:
    ;退出程序
    mov eax, 1
    int 0x80

judge_input:
    xor r10, r10 ;r10w用于存储进制
    xor r11, r11 ;r11b用于存储输入的长度

Integer_loop:
        cmp byte[esi], ' '
        je Integer_done

        cmp byte[esi], '9'
        jg judge_error
        cmp byte[esi], '0'
        jl judge_error

        mov dl, byte[esi]
        mov byte[IntegerBuffer + r11], dl

        inc esi
        inc r11

        jmp Integer_loop

Integer_done:
        add esi, 1
        cmp byte[esi], 'b'
        je binary_type
        cmp byte[esi], 'o'
        je octal_type
        cmp byte[esi], 'h'
        je hexa_type

        jmp judge_error

    binary_type:
        mov r10, 2
        add esi, 1
        cmp byte[esi], 10
        jne judge_error

        jmp judge_done

    octal_type:
        mov r10, 8
        add esi, 1
        cmp byte[esi], 10
        jne judge_error

        jmp judge_done

    hexa_type:
        mov r10, 16
        add esi, 1
        cmp byte[esi], 10
        jne judge_error

        jmp judge_done


    judge_done:
        mov eax, 1
        ret

    judge_error:
        mov eax, 0
        ret



convert_number:
    ;r10存储了进制信息
    ;r11存储了输入的长度
    ;IntegerBuffer存储了输入的整数
    ;r12用于Integer计数
    ;r13用于result计数


    xor r13, r13
    xor ax, ax
    xor dx, dx

convert_loop:
    xor r12, r12
    xor dx, dx
    inner_loop:
        mov ax, dx
        mov r9w, 10
        mul r9w
        movzx bx, byte[IntegerBuffer + r12]
        sub bx, '0' ;转换成数字
        add ax, bx
        xor bx, bx
        xor cx, cx
        xor dx, dx
        div r10b
        movzx dx, ah
        add al, '0' ;转换成字符
        mov byte[IntegerBuffer + r12], al

        inc r12
        cmp r12, r11
        jne inner_loop

    cmp dl, 10
    je hexa_A

    cmp dl, 11
    je hexa_B

    cmp dl, 12
    je hexa_C

    cmp dl, 13
    je hexa_D

    cmp dl, 14
    je hexa_E

    cmp dl, 15
    je hexa_F

    jmp normal

Zhexa_A:
    mov dl, 'a'
    jmp store_result

hexa_B:
    mov dl, 'b'
    jmp store_result

hexa_C:
    mov dl, 'c'
    jmp store_result

hexa_D:
    mov dl, 'd'
    jmp store_result

hexa_E:
    mov dl, 'e'
    jmp store_result

hexa_F:
    mov dl, 'f'
    jmp store_result

normal:
    add dl, '0' ;转换成字符
store_result:
    mov byte[resultBuffer + r13], dl
    inc r13

    call NotZero
    test eax, eax
    jz convert_loop

    cmp r10, 2
    je add_binary

    cmp r10, 8
    je add_octal

    cmp r10, 16
    je add_hexa

    add_binary:
        ;push r13
        mov byte[resultBuffer + r13], 'b'
        inc r13
        mov byte[resultBuffer + r13], '0'
        inc r13
        jmp convert_ret

    add_octal:
        ;push r13
        mov byte[resultBuffer + r13], 'o'
        inc r13
        mov byte[resultBuffer + r13], '0'
        inc r13
        jmp convert_ret

    add_hexa:
        ;push r13
        mov byte[resultBuffer + r13], 'x'
        inc r13
        mov byte[resultBuffer + r13], '0'
        inc r13
        jmp convert_ret

convert_ret:
    call reverse_result
    ret


;获取字符串长度
strlen:
        xor ecx, ecx
        mov edi, esi

    strlen_loop:
        cmp byte [edi + ecx], 0
        je strlen_done
        inc ecx
        jmp strlen_loop

    strlen_done:
        mov eax , ecx
        ret

reverse_result:
    xor r8, r8 ;r8用于temp
    xor r9, r9 ;r9用于temp
    mov r14, r13 ;r14用于反转次数
    mov r15, 0 ;计数
    shr r14, 1

    ;cmp r14, 0 ;原来没加0b之前这里有单独判断
    ;je reverse_ret

reverse_loop:    ;反转结果
    dec r13
    mov r8b, byte[resultBuffer + r13]
    mov r9b, byte[resultBuffer + r15]
    mov byte[resultBuffer + r15], r8b
    mov byte[resultBuffer + r13], r9b
    inc r15

    cmp r15, r14
    jne reverse_loop

reverse_ret:
    ret

NotZero:
    xor r8b, r8b ;r8b计数
    sub r8b, 1
    xor r9b, r9b ;r9b用于temp
NotZero_loop:
    inc r8b
    cmp r8, r11
    je NotZero_done

    cmp byte[IntegerBuffer + r8], '0'
    je NotZero_loop

    mov eax, 0
    ret

NotZero_done:
    mov eax, 1
    ret














