bits 64
default rel

global load_gdt
global load_tss

load_gdt:
    lgdt [rdi]        ; Load the GDT pointer passed in RDI
    
    ; Flush CS (Code Segment) by doing a far return
    push 0x08         ; Push Kernel Code Selector (Offset 0x08 in GDT)
    lea rax, [rel .reload_cs] 
    push rax          ; Push return address
    retfq             ; Far Return (pops IP and CS)

.reload_cs:
    ; Reload data segments with Kernel Data (Offset 0x10)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ret

load_tss:
    ; Load Task Register with offset 0x28 (Index 5 * 8)
    mov ax, 0x28
    ltr ax
    ret