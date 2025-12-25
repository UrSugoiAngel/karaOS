bits 64
default rel  ; Fixes "section-crossing relocation" warnings

extern exception_handler

; --- MACROS ---
%macro isr_err_stub 1
isr_stub_%+%1:
    ; Error code is already pushed by CPU
    push %1                  ; Push interrupt number
    jmp isr_common           ; Jump to common handler
%endmacro

%macro isr_no_err_stub 1
isr_stub_%+%1:
    push 0                   ; Push dummy error code
    push %1                  ; Push interrupt number
    jmp isr_common           ; Jump to common handler
%endmacro

; --- COMMON HANDLER (Saves Context) ---
isr_common:
    ; 1. Save all registers
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rbp
    push rdi
    push rsi
    push rdx
    push rcx
    push rbx
    push rax

    ; 2. Call C handler
    ; RDI is the first argument in System V ABI. 
    ; We pass the stack pointer (RSP) so the C code can access the regs.
    mov rdi, rsp
    call exception_handler

    ; 3. Restore registers
    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop rbp
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15

    ; 4. Cleanup error code and interrupt number (16 bytes)
    add rsp, 16
    iretq

; --- EXCEPTION STUBS (0-31) ---
isr_no_err_stub 0
isr_no_err_stub 1
isr_no_err_stub 2
isr_no_err_stub 3
isr_no_err_stub 4
isr_no_err_stub 5
isr_no_err_stub 6
isr_no_err_stub 7
isr_err_stub    8
isr_no_err_stub 9
isr_err_stub    10
isr_err_stub    11
isr_err_stub    12
isr_err_stub    13
isr_err_stub    14
isr_no_err_stub 15
isr_no_err_stub 16
isr_err_stub    17
isr_no_err_stub 18
isr_no_err_stub 19
isr_no_err_stub 20
isr_no_err_stub 21
isr_no_err_stub 22
isr_no_err_stub 23
isr_no_err_stub 24
isr_no_err_stub 25
isr_no_err_stub 26
isr_no_err_stub 27
isr_no_err_stub 28
isr_no_err_stub 29
isr_err_stub    30
isr_no_err_stub 31

; --- DEFAULT HANDLER FOR UNUSED INTERRUPTS ---
; This was the missing symbol causing your error
isr_stub_default_handler:
    push 0
    push 255          ; Use 255 or similar to indicate "Unknown/Spurious"
    jmp isr_common

; --- IRQ STUBS (32+) ---
; Ideally you should define these individually like above (32-47)
; But for now, we will map them to the default or you can add:
; isr_no_err_stub 32
; isr_no_err_stub 33 ... etc


; --- IDT POINTER TABLE ---
section .data
global isr_stub_table

isr_stub_table:
; Generate pointers for 0-31
%assign i 0 
%rep    32 
    dq isr_stub_%+i
%assign i i+1 
%endrep

; Fill the remaining entries (32 to 255) with the default handler
; FIX: Use (256 - 32) so the table is exactly 256 entries long.
times (256 - 32) dq isr_stub_default_handler