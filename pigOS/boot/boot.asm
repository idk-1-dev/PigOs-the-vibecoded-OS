; ================================================================
; pigOS - POUG Bootloader Stage (Assembly stub for GRUB multiboot)
; Sets up x86_64 long mode then calls kernel_main
; ================================================================
MBOOT_MAGIC    equ 0x1BADB002
MBOOT_FLAGS    equ (1<<0)|(1<<1)
MBOOT_CHECKSUM equ -(MBOOT_MAGIC + MBOOT_FLAGS)

section .multiboot
bits 32
align 4
    dd MBOOT_MAGIC
    dd MBOOT_FLAGS
    dd MBOOT_CHECKSUM

section .bss
align 4096
pml4_table:   resb 4096
pdpt_table:   resb 4096
pd_table0:    resb 4096
pd_table1:    resb 4096
pd_table2:    resb 4096
pd_table3:    resb 4096
stack_bottom: resb 65536   ; 64KB stack
stack_top:

section .rodata
align 8
gdt64:
    dq 0
.code: equ $ - gdt64
    dq (1<<43)|(1<<44)|(1<<47)|(1<<53)
.data: equ $ - gdt64
    dq (1<<41)|(1<<44)|(1<<47)
.ptr:
    dw $ - gdt64 - 1
    dq gdt64

section .text
bits 32
global _start
extern kernel_main

_start:
    cli
    mov [mb_magic], eax
    mov [mb_info],  ebx
    mov esp, stack_top

    ; Clear page tables
    cld
    mov edi, pml4_table
    xor eax, eax
    mov ecx, (6*4096)/4
    rep stosd

    ; PML4[0] -> PDPT
    mov eax, pdpt_table
    or  eax, 0x03
    mov [pml4_table], eax

    ; PDPT[0] -> PD
    mov eax, pd_table0
    or  eax, 0x03
    mov [pdpt_table], eax

    mov eax, pd_table1
    or  eax, 0x03
    mov [pdpt_table + 8], eax

    mov eax, pd_table2
    or  eax, 0x03
    mov [pdpt_table + 16], eax

    mov eax, pd_table3
    or  eax, 0x03
    mov [pdpt_table + 24], eax

    ; PDs: identity map 4GB with 2MB pages
    mov ecx, 0
    mov eax, 0x83
.pd_loop0:
    mov [pd_table0 + ecx*8], eax
    add eax, 0x200000
    inc ecx
    cmp ecx, 512
    jne .pd_loop0

    mov ecx, 0
.pd_loop1:
    mov [pd_table1 + ecx*8], eax
    add eax, 0x200000
    inc ecx
    cmp ecx, 512
    jne .pd_loop1

    mov ecx, 0
.pd_loop2:
    mov [pd_table2 + ecx*8], eax
    add eax, 0x200000
    inc ecx
    cmp ecx, 512
    jne .pd_loop2

    mov ecx, 0
.pd_loop3:
    mov [pd_table3 + ecx*8], eax
    add eax, 0x200000
    inc ecx
    cmp ecx, 512
    jne .pd_loop3

    ; Enable PAE
    mov eax, cr4
    or  eax, (1<<5)
    mov cr4, eax

    ; Load PML4
    mov eax, pml4_table
    mov cr3, eax

    ; EFER.LME
    mov ecx, 0xC0000080
    rdmsr
    or  eax, (1<<8)
    wrmsr

    ; Enable paging
    mov eax, cr0
    or  eax, (1<<31)|(1<<0)
    mov cr0, eax

    lgdt [gdt64.ptr]
    jmp gdt64.code:long_mode_start

bits 64
long_mode_start:
    mov ax, gdt64.data
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov rsp, stack_top

    mov edi, dword [mb_magic]
    mov esi, dword [mb_info]
    call kernel_main

.hang: cli
    hlt
    jmp .hang

section .data
mb_magic: dd 0
mb_info:  dd 0
