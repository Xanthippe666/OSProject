%include "pm.inc"

org 0x9000

VRAM_ADDRESS equ 0x000a0000

jmp LABEL_BEGIN

[SECTION .gdt]

	;    SEGMENT BASE ADDRESS,    SEGMENT LENGTH,     PRIVILEGES/SETTINGS
LABEL_GDT:   Descriptor 0, 0,0
LABEL_DESC_CODE32: Descriptor 0h, SegCode32Len - 1, DA_C + DA_32
LABEL_DESC_VIDEO:  Descriptor 0B8000h, 0FFFFh, DA_DRW
LABEL_DESC_VRAM:   Descriptor 0, 0FFFF_FFFFh,  DA_DRW
LABEL_DESC_STACK:  Descriptor 0, TopOfStack,   DA_DRWA+DA_32

GdtLen equ $ - LABEL_GDT
GdtPtr dw GdtLen - 1
       dd 0h

SelectorCode32 equ LABEL_DESC_CODE32 - LABEL_GDT
SelectorVideo equ  LABEL_DESC_VIDEO  - LABEL_GDT
SelectorStack equ LABEL_DESC_STACK - LABEL_GDT
SelectorVram  equ LABEL_DESC_VRAM   - LABEL_GDT


[SECTION .s16]
[BITS 16]
LABEL_BEGIN:
	;Clear registers
	mov ax, cs   ;move segment C into address of ax
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov sp, 0100h
	
	;Set up VGA output
	mov al, 0x13
	mov ah, 0
	int 0x10

	;Set segment address for LABEL_SEG_CODE32 dynamically
	xor eax, eax   ;clear eax 32-bit register
	mov ax, cs     ;Code segment address put into LABEL_SEG_CODE32 
	shl eax, 4   ;push left by half a byte 
	add eax, LABEL_SEG_CODE32 ;	      
	mov word [LABEL_DESC_CODE32 + 2], ax	
	shr eax, 16   ;shift right by 3 bytes  	
	mov byte [LABEL_DESC_CODE32 + 4], al
	mov byte [LABEL_DESC_CODE32 + 7], ah	

	;Set segment stack for C language
	xor eax, eax
	mov ax, cs
	shl eax, 4
	add eax, LABEL_STACK
	mov word [LABEL_DESC_STACK + 2], ax
	shr eax, 16
	mov byte [LABEL_DESC_STACK + 4], al
	mov byte [LABEL_DESC_STACK + 7], ah	
	
	;Set segment address for LABEL_GDT dynamically
	xor eax, eax
	mov ax, ds
	shl eax, 4
	add eax, LABEL_GDT
	mov dword [GdtPtr + 2], eax
	
	lgdt [GdtPtr]  ;load gdt pointer

	cli ;turn-off interrupts

	in al, 92h  ;internal calls?
	or al, 00000010b
	out 92h, al

	mov eax, cr0  ;load control register data
	or eax, 1     ;Set for protective mode
	mov cr0, eax  ;store control register data	
	
	jmp dword SelectorCode32: 0 ;Jump to the 32-bit code

[SECTION .s32]
[BITS 32]
;Helper functions

LABEL_SEG_CODE32:
	
	;initialize stack
	mov ax, SelectorStack
	mov ss, ax	
	mov esp, TopOfStack
	
	mov ax, SelectorVram
	mov ds, ax

C_CODE_ENTRY:
	%include "write_vga_desktop.asm"
	%include "ioFunc.asm"


	
io_hlt: ;void io_hlt(void);
	HLT
	RET

SegCode32Len equ $ - LABEL_SEG_CODE32     ;$ refers to the current location of the compiler??

[SECTION .gs]
ALIGN 32
[BITS 32]
LABEL_STACK:
times 512 db 0

TopOfStack equ $ - LABEL_STACK
