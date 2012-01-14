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
NAME            DISPLAY_ISR

;
$EJECT

EXTRN	DATA	(DisplayRead, DisplayNext, GameTimer)
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
      						
int2_code_seg   SEGMENT CODE            ; segment for interrupt function
                RSEG    int2_code_seg   ; switch to this code segment
                USING   2               ; register bank for interrupt routine


; timing: worst case 50 cycles + 8*MACRO = 50 + 104 = 154 cycles ~ 2 sound-irq-times
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
                
;------------------------------------------------------------------------------
; The END directive is ALWAYS required.
;------------------------------------------------------------------------------
                END             ; End Of File


