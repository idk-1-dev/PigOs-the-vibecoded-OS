; pigOS FLARE - Colorful bootloader
org 0x7C00
bits 16

s:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    
    ; Save cursor pos
    mov ah, 0x03
    int 0x10
    
    ; Clear screen
    mov ah, 0x07
    mov al, 0
    mov bh, 0x07
    mov cx, 0
    mov dh, 24
    mov dl, 79
    int 0x10
    
    ; Print splash - CYAN
    mov si, splash1
    call print
    ; MAGENTA
    mov si, splash2
    call print
    ; CYAN
    mov si, splash3
    call print
    
    ; Countdown - YELLOW
    mov si, cnt1
    call print
    
    mov cx, 5
.l: mov al, cl
    add al, 48
    mov [cnt_digit], al
    mov si, cnt2
    call print
    
    push cx
    mov cx, 0x7FFF
.d: loop .d
    pop cx
    loop .l
    
    ; GREEN - Booting
    mov si, boot_msg
    call print
    
    xor eax, eax
    mov ax, cs
    shl eax, 4
    mov [g+8], eax
    mov [g+16], eax
    
    in al, 0x92
    or al, 2
    out 0x92, al
    cli
    lgdt [gdtr]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:pm

print:
    lodsb
    or al, al
    jz .e
    mov ah, 0x0E
    int 0x10
    jmp print
.e: ret

; Color codes
splash1: db 0x1B,'[1;36m','  +==============================+',0x0D,0x0A,0
splash2: db 0x1B,'[1;35m','  |      pigOS FLARE Boot       |',0x0D,0x0A,0
splash3: db 0x1B,'[1;36m','  +==============================+',0x1B,'[0m',0x0D,0x0A,0
cnt1: db 0x1B,'[1;33m','  Boot in: ',0
cnt_digit: db '5',0
cnt2: db 's  ',0x0D,0x0A,0
boot_msg: db 0x1B,'[1;32m','  [BOOT] Loading kernel...',0x1B,'[0m',0x0D,0x0A,0

g: dq 0,0
.code: dw 0xFFFF,0,0,10011010b,11001111b,0
.data: dw 0xFFFF,0,0,10010010b,11001111b,0
gdtr: dw 23
gw: dw g

bits 32

pm: mov ax, 0x10
    mov ds, ax
    mov ss, ax
    mov esp, 0x90000
    
    push 0x2BADB002
    call 0x100000
    jmp $

times 444-($-$$) db 0
dw 0xAA55
