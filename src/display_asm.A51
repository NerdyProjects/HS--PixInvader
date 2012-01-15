;------------------------------------------------------------------------------
; Source code template for A251/A51 assembler modules.
; Copyright (c) 1995-2000 KEIL Software, Inc.
;------------------------------------------------------------------------------
$NOMOD51                ; disable predefined 8051 registers
#include <atmel/regx52.h>      // include CPU definition file (for example, 8052)
#include "display_def.h"

sfr AUXR1	= 0xA2;
sfr	DP1L	= 0x84;
sfr	DP1H	= 0x85;

;------------------------------------------------------------------------------
; Module name (optional)
;------------------------------------------------------------------------------
NAME            ___DISPLAY_ASM

;
$EJECT

?PR?timer2_isr?___DISPLAY_ASM   SEGMENT CODE            ; segment for interrupt function
				
?PR?_displayPixel?___DISPLAY_ASM             SEGMENT CODE	

PUBLIC		_displayPixel			
					

EXTRN	DATA	(DisplayRead, DisplayNext, DisplayWrite, GameTimer)
EXTRN	BIT		(BufferSwitchRequest)                                                                
EXTRN	XDATA	(DisplaySelectReg, DisplayDataReg)

color		EQU		R7
col_x8		EQU		R6

;display_isr_seg	SEGMENT	DATA
;				RSEG	display_isr_seg
;color:			DS	1
;col:			DS	1

; This is used in display irq code. SelectReg and DataPtrReg are incremented
; so it can be called in a row with the same parameters.
; Outputs data at DataPtrReg to matrix latch.
; 13 cycles
OUTPUT_COLUMN	MACRO	SelectAdr, DataAdr, SelectReg, DataPtrReg

				MOV		A, SelectReg
				MOV		DPTR, #SelectAdr
				MOVX	@DPTR, A
				INC		SelectReg
				
				MOVX	A, @DataPtrReg			; get display data byte
				INC		DataPtrReg
				MOV		DPTR, #DataAdr
				MOVX	@DPTR, A
				
				ENDM
				

				
;------------------------------------------------------------------------------
; To include an interrupt service routins, provide an LJMP to the ISR at the
; interrupt vector address.
;------------------------------------------------------------------------------
                CSEG    AT  2BH         ; 0BH is address for Timer 0 interrupt
                LJMP    timer2int
      						

                RSEG    ?PR?timer2_isr?___DISPLAY_ASM   ; switch to this code segment
                USING   2               ; register bank for interrupt routine


; timing: worst case 50 cycles + 8*MACRO = 50 + 104 = 154 cycles ~ 2 sound-irq-times
;
; second DPTR would save about 15 cycles, but would enforce
; PUSH/POP of AUXR1 and set to 0 in sound irq -> 6 cycles more, but they are executed
; 2 times as often -> does not really take us anywhere

timer2int:      PUSH    PSW			
                MOV     PSW,#10H        ; register bank 2
                PUSH    ACC
                PUSH	DPH
                PUSH	DPL

				MOV		A, color
				JZ		color_0
				MOV		A, #(DISPLAY_MATRICES * DISPLAY_COLS_PER_MATRIX)
				
color_0:		ADD		A, DisplayRead	; A is zero when we jump here, else we have COLOR offset
				ADD		A, col_x8
				MOV		R0, A			; calculated read index is in R0
				MOV		R1, #DISPLAY_BLANK	
				
				OUTPUT_COLUMN	DisplaySelectReg, DisplayDataReg, R1, R0
				OUTPUT_COLUMN	DisplaySelectReg, DisplayDataReg, R1, R0
				OUTPUT_COLUMN	DisplaySelectReg, DisplayDataReg, R1, R0
				OUTPUT_COLUMN	DisplaySelectReg, DisplayDataReg, R1, R0
				OUTPUT_COLUMN	DisplaySelectReg, DisplayDataReg, R1, R0
				OUTPUT_COLUMN	DisplaySelectReg, DisplayDataReg, R1, R0
				OUTPUT_COLUMN	DisplaySelectReg, DisplayDataReg, R1, R0
				OUTPUT_COLUMN	DisplaySelectReg, DisplayDataReg, R1, R0
				
				MOV		A, col_x8
				
				MOV		DPTR, #DisplaySelectReg
				MOVX	@DPTR, A			; output (col << 3) to SelectRegister
				
				SUBB	A, #(DISPLAY_COLS_PER_MATRIX * 7)		; carry is clear, last ADD never overflows
				JZ		columns_fin
				; A is now col_x8 - 56, carry is set
				; Add 64 so we add 8 in total.
				ADD		A, #64
				MOV		col_x8, A
				JMP		fin
				
columns_fin:	MOV		col_x8, #0					; finished all columns, continue with next color
				MOV		A, color
				JZ		colors_fin
				INC		color
				JMP		fin								
				
colors_fin:		MOV		color, #0					; finished all columns with all colors, do some work:
				
				INC		GameTimer					; Increment application timebase
													; *todo* key readin should happen here!
				
				
				
				JNB		BufferSwitchRequest, fin
				; we got a request to switch to next read buffer
				MOV		A, DisplayRead
				XCH		A, DisplayNext
				MOV		DisplayRead, A
				CLR		BufferSwitchRequest
								
fin:				

				POP		DPL
				POP		DPH
                POP     ACC
                POP     PSW
                RETI
                
                
; ----------------------------------------------------------------------------
; displayPixel
; ----------------------------------------------------------------------------
;                
; void displayPixel(unsigned char x, unsigned char y, unsigned char color);
;                                R7               R5                 R3     
; cycles: max. 42 + (3*(y%7)) for the shift -> average case ~51 cycles
; *yay* got ~half the cycles of KEIL :)       
				
				RSEG	?PR?_displayPixel?___DISPLAY_ASM
				USING 0

_displayPixel:				
				MOV		B, #DISPLAY_COLS_PER_MATRIX
				MOV		A, R7
				DIV		AB
				ADD		A, DisplayWrite
				MOV		R0, A				; R0 = &DisplayWrite[x/DISPLAY_COLS_PER_MATRIX]
				MOV		A, B				; A = x % DISPLAY_COLS_PER_MATRIX
				SWAP	A					; multiply by 8 (DISPLAY_MATRICES)
				RRC		A					; carry cleared by previous DIV
				ADD		A, R0
				MOV		R0, A				; R0 = &DisplayWrite[x/DISPLAY_COLS_PER_MATRIX + DISPLAY_MATRICES*(x%DISPLAY_COLS_PER_MATRIX)]
				MOV		A, R5
				SUBB	A, #7				; carry clear by last addition
				JC		y_smaller_7
				MOV		R5, A				; y % 7 in R5 (for y <= 13)
				MOV		A, R0				; when y >= 7: add 4 to address
				ADD		A, #4
				MOV		R0, A				

y_smaller_7:	INC		R5					; we can only efficiently test for 0
				MOV		A, #1
				CLR		C
				JMP		loop_test			; begin with test, maybe we do not want to shift

loop_shift:		RLC		A
loop_test:		DJNZ	R5,loop_shift
											
				MOV		R5, A				; R5 = 1 << (y%7)
				
				MOV		A, R3
				JNB		ACC.0, high_color
				MOVX	A, @R0
				ORL		A, R5
				MOVX	@R0, A
				
high_color:		MOV		A, R3
				JNB		ACC.1, finished
				MOV		A, #DISPLAY_BUFFER_BYTES_PER_COLOR
				ADD		A, R0
				MOV		R0, A				; Increment pointer by color offset
				MOVX	A, @R0
				ORL		A, R5
				MOVX	@R0, A
				
finished:		RET				
				                
                
;------------------------------------------------------------------------------
; The END directive is ALWAYS required.
;------------------------------------------------------------------------------
                END             ; End Of File

