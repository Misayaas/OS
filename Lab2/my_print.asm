section .text
    ;global _start
    global asm_print

asm_print:
;_start:
    push ebp
    mov ebp, esp
    mov ecx, [ebp + 8]
    call strlen
    mov edx, eax
    mov eax, 4
    mov ebx, 1
    int 0x80
    pop ebp
    ret

strlen:
    mov eax, 0 ;用于计算长度
loop:
    cmp byte[ecx + eax], 0
    je finish_loop
    inc eax
    jmp loop
finish_loop:
    ret

;section .text
    ;global asm_print

;asm_print:
    ;push ebp
    ;mov ebp, esp
    ;mov edx, [ebp+12]
    ;mov ecx, [ebp+8]
    ;mov ebx, 1
    ;mov eax, 4
    ;int 80h
    ;pop ebp
    ;ret


