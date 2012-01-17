;------------------------------------------------------------------------------
; Source code template for A251/A51 assembler modules.
; Copyright (c) 1995-2000 KEIL Software, Inc.
;------------------------------------------------------------------------------
$NOMOD51                ; disable predefined 8051 registers
#include <atmel/regx52.h>      // include CPU definition file (for example, 8052)
#include "spi.h"

;------------------------------------------------------------------------------
; Module name (optional)
;------------------------------------------------------------------------------
NAME            ___SPI_ASM

;
$EJECT

?PR?get4Bytes?___SPI_ASM   SEGMENT CODE          
				
PUBLIC		get4Bytes			
					

READ_BYTE		MACRO	TEMP_REG
				LOCAL	LOOP_SCK_LOW, LOOP_SCK_HIGH
	
				MOV 	TEMP_REG, #8			;    2 cycles setup time + 
												;    2 cycles result movement
LOOP_SCK_LOW:   JB		SCK, LOOP_SCK_LOW   	;    8 cycles per bit needed
				MOV		C, MOSI					;    -> 68 cycles / 8 bit
				RLC		A						;    -> fCPU has to be >= 102 * fSCK
LOOP_SCK_HIGH:	JNB		SCK, LOOP_SCK_HIGH
				DJNZ 	TEMP_REG, LOOP_SCK_LOW

				ENDM
								

; ----------------------------------------------------------------------------
; get4Bytes
; ----------------------------------------------------------------------------
;                
; unsigned long get4Bytes(void)
				
				RSEG	?PR?get4Bytes?___SPI_ASM
				USING 0
				$REGUSE get4Bytes(R2,R4,R5,R6,R7)

get4Bytes:		
				READ_BYTE	r2
				MOV		R7, A		; R7 is LSB of return long
				READ_BYTE	r2
				MOV		R6, A
				READ_BYTE	r2
				MOV		R5, A
				READ_BYTE	r2
				MOV		R4, A		; R4 is MSB of return long			
				
				
finished:		RET				
				                
                
;------------------------------------------------------------------------------
; The END directive is ALWAYS required.
;------------------------------------------------------------------------------
                END             ; End Of File


