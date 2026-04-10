; ================================================================
; pigOS POUG Bootloader Stage 2 - Protected Mode Setup
; Loaded at 0x10000 by stage 1
; ================================================================
org 0x10000
bits 16

entry:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    
    call enable_a20
    call setup_gdt
    call enter_pmode
    
enable_a20:
    in al, 0x92
    or al, 2
    out 0x92, al
    ret

setup_gdt:
    xor eax, eax
    mov ax, cs
    shl eax, 4
    mov dword [gdt_code - entry + gdt_base], eax
    mov dword [gdt_base + 8], eax
    mov dword [gdt_base + 12], 0
    
    xor eax, eax
    mov ax, ds
    shl eax, 4
    mov dword [gdt_base + 16], eax
    mov dword [gdt_base + 20], 0
    
    ret

enter_pmode:
    lgdt [gdtr]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:pmode

gdt_base:
gdt:
    dq 0
gdt_code:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10011010b
    db 11001111b
    db 0x00
gdt_data:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b
    db 11001111b
    db 0x00
gdt_end:

gdtr:
    dw gdt_end - gdt - 1
    dd gdt

bits 32

pmode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000
    
    mov edi, 0xB8000
    mov ecx, 80*25
    mov ax, 0x0720
    rep stosw
    
    mov edi, (80*2)*1 + 2*2
    mov esi, msg_boot
.loop:
    lodsb
    or al, al
    jz .done
    mov ah, 0x07
    mov [edi], ax
    add edi, 2
    jmp .loop
.done:
    
    push dword 0
    push dword 0x2BADB002
    call 0x100000
    
    jmp $

msg_boot:
    db '  [POUG] pigOS v1.0 - Entering kernel...', 0
