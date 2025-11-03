; boot.asm
; nasm -f bin boot.asm -o boot.bin
org 0x7C00
bits 16

start:
    cli
    xor ax, ax
    mov ds, ax
    mov ss, ax
    mov sp, 0x7C00

    mov si, bootmsg
    call print_string

    ; --- E820 memory map probe ---
    ; We'll store entries at physical 0x7000 (ES:DI = 0000:7000)
    xor ebx, ebx           ; continuation = 0
    mov di, 0x7000
    xor ax, ax
    mov cx, 24             ; buffer size
    mov dx, 0x534D         ; 'SM' lower word
    mov dx, 0x534D         ; we'll set EDX properly below
    ; Set EDX = 'SMAP'  (0x534D4150) - do via mov eax, 'PAMS' then swap? simpler:
    mov eax, 0x50414D53    ; 'PAMS' little? actually this places 0x50414D53 which is 'PAM S'... safer load as immediate parts:
    ; Instead put 'SMAP' into EDX by loading from memory
    mov dx, 'M'            ; placeholder to satisfy assembler (we'll set EDX via mov instruction)
    ; Use two instructions to set EDX = 'SMAP'
    mov edx, 0x534D4150    ; 'SMAP' (0x53='S',0x4D='M',0x41='A',0x50='P')
.E820_LOOP:
    push bx
    mov ax, 0xE820
    mov cx, 24
    mov es, 0
    mov di, di             ; DI already set
    int 0x15
    jc .E820_DONE          ; if CF set -> stop
    ; returned: entry stored at ES:DI (we used ES=0, DI points to buffer)
    add di, 24
    pop bx
    ; EBX is continuation; if EBX==0 then done
    cmp ebx, 0
    jne .E820_LOOP

.E820_DONE:
    ; calculate count: number of entries = (DI - 0x7000) / 24
    mov ax, di
    sub ax, 0x7000
    mov bx, 24
    xor dx, dx
    div bx                ; ax = count
    ; store count at 0x6FF0
    mov word [0x6FF0], ax

    ; --- load kernel to 0x1000:0000 (physical 0x10000) ---
    mov si, loadmsg
    call print_string

    mov ah, 0x02          ; INT13 read sectors
    mov al, 20            ; number of sectors to read (change if kernel larger)
    mov ch, 0
    mov cl, 2             ; start at sector 2
    mov dh, 0
    mov dl, [BOOT_DRIVE]
    mov bx, 0             ; offset in ES
    mov es, 0x1000        ; ES:BX -> 0x1000:0000 physical 0x10000
    mov bp, bx

    ; We'll read sectors one by one into ES:BX with incrementing BX
    mov si, 0
    mov cx, al            ; sector count
.read_sectors:
    mov ah, 0x02
    mov al, 1
    mov ch, 0
    mov cl, byte [cur_sector]
    mov dl, [BOOT_DRIVE]
    int 0x13
    jc disk_err
    ; advance ES:BX (we used ES fixed and BX offset)
    add bx, 512
    add byte [cur_sector], 1
    dec cx
    jne .read_sectors

    mov si, loadedmsg
    call print_string

    ; --- enter protected mode ---
    cli
    lgdt [gdt_desc]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:pm_entry

disk_err:
    mov si, diskmsg
    call print_string
    jmp $

; BIOS helpers
print_string:
    mov ah, 0x0E
.loop_ps:
    lodsb
    or al, al
    jz .ps_done
    int 0x10
    jmp .loop_ps
.ps_done:
    ret

; protected mode entry
[bits 32]
pm_entry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000
    ; call kernel at physical 0x10000
    call 0x10000
    hlt
    jmp $

; GDT
gdt_start:
    dq 0
    dq 0x00CF9A000000FFFF
    dq 0x00CF92000000FFFF
gdt_end:
gdt_desc:
    dw gdt_end - gdt_start - 1
    dd gdt_start

BOOT_DRIVE db 0
bootmsg db "CinnaOS boot: probing memory (E820)...",0x0D,0x0A,0
loadmsg db "Loading kernel...",0x0D,0x0A,0
loadedmsg db "Kernel loaded.",0x0D,0x0A,0
diskmsg db "Disk read error",0x0D,0x0A,0

cur_sector db 2

times 510-($-$$) db 0
dw 0xAA55
