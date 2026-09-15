;
; Copyright (c) 2019-2026 luke8086.
; Distributed under the terms of GPL-2 License.
;
; File: boot_a.s - Combined 2-stage bootloader, assembly parts
;

[cpu 386]
[bits 16]

STAGE2_DEST         equ 0x7e00
STAGE2_START_LBA    equ 2

KERNEL_DEST         equ 0x10000

extern stage2_cmain

[section .asm]

;;
;; Stage 1
;;

;
; Entry point
;

    jmp 0x0000:stage1_main


;
; Global variables filled by mkdisk, must stay at fixed offset
;

global stage2_sectors:data
stage2_sectors: dw 0


;
; Global variables
;

global boot_drive_index:data
boot_drive_index    db 0
boot_drive_spt      db 9
boot_drive_heads    db 2

remaining_sectors   dw 0

current_lba         dw STAGE2_START_LBA
current_dest_seg    dw STAGE2_DEST >> 4
current_cylinder    dw 0
current_head        db 0
current_sector      db 0
current_count       db 0


;
; Stage 1 main function
;

stage1_main:
    ; Setup segments and stack
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, 0xfff0
    sti

    ; Clear direction flag just in case
    cld

    ; Initialize boot drive
    mov [boot_drive_index], dl
    o32 call dword reset_drive
    o32 call dword load_drive_geometry

    ; Load stage 2 loader
    mov ax, [stage2_sectors]
    mov [remaining_sectors], ax
    o32 call dword safe_load_remaining_sectors
    jmp stage2_main


;
; Halt the program
;

halt:
    cli
    hlt
    jmp halt


;
; Return the smaller of AX and BX in AX
;

get_min_word:
    cmp ax, bx
    jbe .done
    mov ax, bx
.done:
    o32 ret


;
; Print a single character
;

global print_char:function
print_char:
    push ebp
    mov ebp, esp
    pushad

    mov ah, 0x0e
    mov al, [ebp + 8]
    xor bx, bx
    int 0x10

    popad
    pop ebp
    o32 ret


;
; Reset the boot drive
;

reset_drive:
    pushad

    xor ah, ah
    mov dl, [boot_drive_index]
    int 0x13

    popad
    o32 ret


;
; Load the boot drive geometry
;

load_drive_geometry:
    pushad
    push es

    ; ES:DI = 0 to work around buggy BIOSes
    xor di, di
    mov es, di

    mov ah, 0x08
    mov dl, [boot_drive_index]
    int 0x13

    ; On error keep the default geometry
    jc .done

    ; SPT = CL & 0x3f
    mov al, cl
    and al, 0x3f

    ; HEADS = DH + 1
    mov ah, dh
    inc ah

    ; Ignore invalid geometry
    test al, al
    jz .done
    test ah, ah
    jz .done

    mov [boot_drive_spt], al
    mov [boot_drive_heads], ah

.done:
    pop es
    popad
    o32 ret


;
; Update current CHS using current LBA
;

update_current_chs:
    pushad

    movzx cx, byte [boot_drive_spt]
    mov ax, [current_lba]
    xor dx, dx
    div cx                              ; AX = LBA / SPT, DX = LBA % SPT
    inc dl
    mov [current_sector], dl            ; sector = LBA % SPT + 1

    movzx cx, byte [boot_drive_heads]
    xor dx, dx
    div cx                              ; AX = AX / heads, DX = AX % heads
    mov [current_cylinder], ax          ; cylinder = (LBA / SPT) / heads
    mov [current_head], dl              ; head = (LBA / SPT) % heads

    popad
    o32 ret


;
; Load sectors from the boot drive using current count, CHS and dest
;

load_sectors:
    push ebx
    push es

    ; CL = ((cylinder >> 2) & 0xc0) | (sector & 0x3f)
    mov dx, [current_cylinder]
    shr dx, 2
    and dl, 0xc0
    mov cl, [current_sector]
    and cl, 0x3f
    or cl, dl

    mov ah, 0x02
    mov al, [current_count]
    mov ch, [current_cylinder]
    mov dh, [current_head]
    mov dl, [boot_drive_index]
    mov es, [current_dest_seg]
    xor bx, bx

    int 0x13

    jc .error

    movzx eax, byte [current_count]
    jmp .done

.error:
    xor eax, eax

.done:
    pop es
    pop ebx
    o32 ret


;
; Call load_sectors up to 3 times and report error on failure
;

safe_load_sectors:
    push ebx

    mov ebx, 3

.retry:
    o32 call dword load_sectors

    test eax, eax
    jnz .success

    o32 call dword reset_drive
    dec ebx
    jnz .retry

    push dword 'E'
    o32 call dword print_char
    add esp, 4

    jmp halt

.success:
    push dword '.'
    o32 call dword print_char
    add esp, 4

    pop ebx
    o32 ret


;
; Load current_count with remaining_sectors and limit it to not exceed:
; - remaining sectors in the current track
; - remaining sectors to the 64KB boundary
; - the BIOS limit of 127
;

update_current_count:
    pushad

    mov ax, [remaining_sectors]

    ; AX = min(AX, SPT - (sector - 1))
    movzx bx, byte [boot_drive_spt]
    movzx dx, byte [current_sector]
    sub bx, dx
    inc bx
    o32 call dword get_min_word

    ; AX = min(AX, (0x1000 - (dest_segment & 0xfff)) / 32)
    ; 64KB = 0x1000 paragraphs, one sector = 32 paragraphs
    mov dx, [current_dest_seg]
    and dx, 0x0fff
    mov bx, 0x1000
    sub bx, dx
    shr bx, 5
    o32 call dword get_min_word

    ; AX = min(AX, 127)
    mov bx, 127
    o32 call dword get_min_word

    mov [current_count], al

    popad
    o32 ret


;
; Keep loading sectors until remaining_sectors is 0
;

safe_load_remaining_sectors:
    pushad

.loop:
    cmp word [remaining_sectors], 0
    je .done

    o32 call dword update_current_chs
    o32 call dword update_current_count
    o32 call dword safe_load_sectors

    mov cx, ax
    sub [remaining_sectors], cx     ; remaining_sectors -= count
    add [current_lba], cx           ; current_lba += count
    shl cx, 5
    add [current_dest_seg], cx      ; current_dest_seg += count * 32

    jmp .loop

.done:
    popad
    o32 ret


;
; MBR partition table with a single bootable partition
;

global mbr_partition_table:data
mbr_partition_table:
    times 0x1be - ($ - $$) db 0
    db 0x80, 0x00, 0x02, 0x00
    db 0x01, 0x00, 0x3f, 0x00
    dd 0x01, 0x7f


;
; Boot-loader designator
;

global mbr_designator:data
mbr_designator:
    times 0x1fe - ($ - $$) db 0
    dw 0b10101010_01010101


;;
;; Stage 2
;;


;
; Global variables filled by mkdisk and mkinitrd, must stay at fixed offset
;

global kernel_sectors:data
kernel_sectors: dw 0

global initrd_sectors:data
initrd_sectors: dw 0

global boot_flags:data
boot_flags: dw 0


;
; Stage 2 main function
;

global stage2_main:function
stage2_main:
    ; Print progress dot
    push dword '.'
    o32 call dword print_char
    add esp, 4

    ; Save timer ticks
    o32 call dword get_ticks
    mov [boot_start_ticks], eax

    ; Call C code
    o32 call dword stage2_cmain

    ; Try enabling A20 using BIOS
    mov ax, 0x2401
    int 0x15

    ; Load the amount of lower memory
    int 0x12
    movzx eax, ax
    mov [mboot_info + 4], eax

    ; Load the amount of upper memory
    clc
    mov ah, 0x88
    int 0x15
    jc .skip_upper_mem
    movzx eax, ax
    mov [mboot_info + 8], eax
.skip_upper_mem:

    ; Disable interrupts
    cli

    ; Load GDT
    lgdt [gdt_pointer]

    ; Enable protected mode
    mov eax, cr0
    or eax, 0x01
    mov cr0, eax

    ; Jump to 32-bit code
    jmp dword 0x08:.code32

[bits 32]

.code32:
    ; Load 32-bit data segment selectors
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0xfff0;

    ; Save pointer to mboot info and jump to the kernel
    mov ebx, mboot_info
    jmp 0x08:KERNEL_DEST

[bits 16]


;
; Read a 16-bit word from the given far pointer
;

global get_far_word:function
get_far_word:
    push ebp
    mov ebp, esp
    push ds
    push esi

    mov si, [ebp + 12]
    mov ax, [ebp + 8]
    mov ds, ax
    movzx eax, word [si]

    pop esi
    pop ds
    pop ebp
    o32 ret


;
; Return BIOS timer tick count
;

global get_ticks:function
get_ticks:
    push ecx
    push edx

    xor ah, ah
    int 0x1a

    mov ax, cx
    shl eax, 16
    mov ax, dx

    pop edx
    pop ecx
    o32 ret


;
; Return ticks elapsed since start of stage 2
;

global get_elapsed_ticks:function
get_elapsed_ticks:
    o32 call dword get_ticks
    cmp eax, [boot_start_ticks]
    jb .done ; In case of rollover, don't subtract start ticks
    sub eax, [boot_start_ticks]
.done:
    o32 ret


;
; Check if a keystroke is waiting
;

global has_key:function
has_key:
    mov ah, 0x01
    int 0x16
    setnz al
    movzx eax, al
    o32 ret


;
; Wait for a keystroke and return ASCII code
;

global get_key:function
get_key:
    xor ah, ah
    int 0x16
    movzx eax, al
    o32 ret


;
; Stop FDD motor and update FDD status in BIOS
;

BDA_SEGMENT             equ 0x0040
BDA_MOTOR_STATUS        equ 0x3f   ; bits 0-3: motors running
BDA_MOTOR_TIMEOUT       equ 0x40
FDC_PORT_DOR            equ 0x3f2
FDC_DOR_MOTORS_OFF      equ 0x0c   ; DMA/IRQ on, not in reset, motors off

global stop_floppy_motor:function
stop_floppy_motor:
    push eax
    push edx
    push es

    mov dx, FDC_PORT_DOR
    mov al, FDC_DOR_MOTORS_OFF
    out dx, al

    mov ax, BDA_SEGMENT
    mov es, ax
    and byte [es:BDA_MOTOR_STATUS], 0xf0
    mov byte [es:BDA_MOTOR_TIMEOUT], 0

    pop es
    pop edx
    pop eax
    o32 ret


;
; Print a null-terminated string
;

global print_str:function
print_str:
    push ebp
    mov ebp, esp
    pushad

    mov esi, [ebp + 8]

.loop:
    lodsb
    test al, al
    jz .done

    mov ah, 0x0e
    xor bx, bx
    int 0x10

    jmp .loop

.done:
    popad
    pop ebp
    o32 ret


;
; Print an unsigned 16-bit integer in decimal
;

global print_ushort:function
print_ushort:
    push ebp
    mov ebp, esp
    pushad

    ; Push digits in reverse order, save count in CX
    mov eax, [ebp + 8]
    mov bx, 10
    xor cx, cx
.divide:
    xor dx, dx
    div bx
    push dx
    inc cx
    test ax, ax
    jnz .divide

    ; Pop digits and print
.print:
    pop ax
    add al, '0'
    mov ah, 0x0e
    xor bx, bx
    int 0x10
    loop .print

    popad
    pop ebp
    o32 ret


;
; C interface to safe_load_remaining_sectors (FIXME: which should be refactored)
;

global safe_load_remaining_sectors_c:function
safe_load_remaining_sectors_c:
    push ebp
    mov ebp, esp
    pushad

    mov ax, [ebp + 8]
    mov [current_dest_seg], ax
    mov ax, [ebp + 12]
    mov [current_lba], ax
    mov ax, [ebp + 16]
    mov [remaining_sectors], ax
    o32 call dword safe_load_remaining_sectors

    popad
    pop ebp
    o32 ret


;
; Copy given number of words to upper memory using BIOS interrupt 15h/87h
;

global copy_ext_mem:function
copy_ext_mem:
    push ebp
    mov ebp, esp
    pushad
    push es

    ; Fill the source descriptor base
    mov eax, [ebp + 12]
    mov [copy_ext_mem_gdt + 16 + 2], ax
    shr eax, 16
    mov [copy_ext_mem_gdt + 16 + 4], al
    mov [copy_ext_mem_gdt + 16 + 7], ah

    ; Fill the target descriptor base
    mov eax, [ebp + 8]
    mov [copy_ext_mem_gdt + 24 + 2], ax
    shr eax, 16
    mov [copy_ext_mem_gdt + 24 + 4], al
    mov [copy_ext_mem_gdt + 24 + 7], ah

    ; ES:SI = descriptor table, CX = word count
    xor ax, ax
    mov es, ax
    mov si, copy_ext_mem_gdt
    mov cx, [ebp + 16]
    mov ah, 0x87
    int 0x15

    ; Some buggy BIOSes may leave interrupts disabled
    sti

    jc .error
    test ah, ah
    jz .done

.error:
    push dword 'X'
    o32 call dword print_char
    add esp, 4
    jmp halt

.done:
    pop es
    popad
    pop ebp
    o32 ret


;
; Load the VBE controller info to the given buffer
; Return 1 on success, 0 on failure
;

global vbe_load_ctrl_info:function
vbe_load_ctrl_info:
    push ebp
    mov ebp, esp
    pushad
    push es

    ; ES:DI = dest buffer
    xor ax, ax
    mov es, ax
    mov di, [ebp + 8]

    ; Load VBE 2.0 information
    mov dword [es:di], 'VBE2'
    mov ax, 0x4f00
    int 0x10

    ; Set return value in the EAX slot saved by pushad
    cmp ax, 0x004f
    sete al
    movzx eax, al
    mov [ebp - 4], eax

    pop es
    popad
    pop ebp
    o32 ret


;
; Load info of the given VBE mode to the given buffer
; Return 1 on success, 0 on failure
;

global vbe_load_mode_info:function
vbe_load_mode_info:
    push ebp
    mov ebp, esp
    pushad
    push es

    ; ES:DI = destination buffer
    xor ax, ax
    mov es, ax
    mov di, [ebp + 12]

    ; Load info
    mov cx, [ebp + 8]
    mov ax, 0x4f01
    int 0x10

    ; Set return value in the EAX slot saved by pushad
    cmp ax, 0x004f
    sete al
    movzx eax, al
    mov [ebp - 4], eax

    pop es
    popad
    pop ebp
    o32 ret


;
; Set the given video mode with a linear framebuffer
; Return 1 on success, 0 on failure
;

global vbe_set_mode:function
vbe_set_mode:
    push ebp
    mov ebp, esp
    pushad
    push es

    ; Set mode
    mov bx, [ebp + 8]
    or bx, (1 << 14) ; Request linear fb
    mov ax, 0x4f02
    int 0x10

    ; Set return value in the EAX slot saved by pushad
    cmp ax, 0x004f
    sete al
    movzx eax, al
    mov [ebp - 4], eax

    pop es
    popad
    pop ebp
    o32 ret


;
; Global variables
;

global boot_start_ticks:data
boot_start_ticks: dd 0


;
; Multiboot info
;

global mboot_info:data
mboot_info:
    dd 0x201                ; flags (mem | bootloader)
    dd 0                    ; mem_lower
    dd 0                    ; mem_upper
    dd 0                    ; boot_device (unused)
    dd 0                    ; cmdline
    times 11 dd 0           ; unused
    dd mboot_loader_name    ; boot_loader_name
    times 5 dd 0            ; unused
    times 7 dd 0            ; framebuffer

mboot_loader_name:  db `GentleBoot\0`


;
; GDT for copy_ext_mem
;

align 16
copy_ext_mem_gdt:
    ; Zeros used by BIOS
    times 16 db 0

    ; Source segment (base addr filled by copy_ext_mem)
    dw 0xFFFF     ; segment limit[15:0]
    dw 0x0000     ; base addr[15:0]
    db 0x00       ; base addr[23:16]
    db 10010011b  ; P, DPL, 1, 0, E, W, A
    db 00000000b  ; G, B, _, AVL, segment limit[19:16]
    db 0x00       ; base addr[31:24]

    ; Target segment (base addr filled by copy_ext_mem)
    dw 0xFFFF     ; segment limit[15:0]
    dw 0x0000     ; base addr[15:0]
    db 0x00       ; base addr[23:16]
    db 10010011b  ; P, DPL, 1, 0, E, W, A
    db 00000000b  ; G, B, _, AVL, segment limit[19:16]
    db 0x00       ; base addr[31:24]

    ; Zeros used by BIOS to build CS and SS descriptors
    times 16 db 0


;
; Temporary GDT, same as in kernel
;

align 16
global gdt:data
gdt:
    ; Null segment
    dw 0x00       ; segment limit[15:0]
    dw 0x00       ; base addr[15:0]
    db 0x00       ; base addr[23:16]
    db 00000000b  ; P, DPL, S, type
    db 00000000b  ; G, DB, _, AVL, segment limit[19:16]
    db 0x00       ; base addr[31:24]

    ; Code segment
    dw 0xFFFF     ; segment limit[15:0]
    dw 0x0000     ; base addr[15:0]
    db 0x00       ; base addr[23:16]
    db 10011010b  ; P, DPL, 1, 1, C, R, A
    db 11001111b  ; G, D, L, AVL, segment limit[19:16]
    db 0x00       ; base addr[31:24]

    ; Data segment
    dw 0xFFFF     ; segment limit[15:0]
    dw 0x00       ; base addr[15:0]
    db 0x00       ; base addr[23:16]
    db 10010010b  ; P, DPL, 1, 0, E, W, A
    db 11001111b  ; G, B, _, AVL, segment limit[19:16]
    db 0x00       ; base addr[31:24]

gdt_pointer:
    dw (3 * 8 - 1)          ; limit (3 descriptors * 8 bytes - 1)
    dd gdt     ; base (pointer to the GDT)

