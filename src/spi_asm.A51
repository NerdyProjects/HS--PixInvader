;------------------------------------------------------------------------------
; low level SPI implementation for 8051 microcontrollers.
; send and receive four bytes at once with C API.
;
; 2012 by Matthias Larisch
;
; fSCK <= fCPU/112 for reading,
; fSCK <= fCPU/212 for writing.
;
; additionally there are two optimized XDATA and DATA read byte commands
; implemented. See get4Bytes_command
;
; this file is based on:
; Source code template for A251/A51 assembler modules.
; Copyright (c) 1995-2000 KEIL Software, Inc.
; 
; 
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
?PR?get4Bytes_command?___SPI_ASM	SEGMENT CODE   
				
PUBLIC		get4Bytes	
PUBLIC		get4Bytes_command		


; loops while SCK is high				
L_SCK_HIGH		MACRO
				LOCAL LOOP
LOOP:			JB		SCK, LOOP	
				ENDM
				
; loops while SCK is low				
L_SCK_LOW		MACRO
				LOCAL LOOP
LOOP:			JNB		SCK, LOOP	
				ENDM
	
READ_BITS		MACRO	TEMP_REG, BITS
				LOCAL	LOOP_SCK_LOW, LOOP_SCK_HIGH, LOOP_BIT
	
				MOV 	TEMP_REG, BITS			;    2 cycles setup time + 
LOOP_BIT:		CPL		MISO					;    2 cycles result movement
				L_SCK_LOW   					;    8 cycles per bit needed
												;    1 cycle per bit for output complement
				MOV		C, MOSI					;    -> 74 cycles / 8 bit
				RLC		A						;    -> fCPU has to be >= 111 * fSCK
				L_SCK_HIGH
				DJNZ 	TEMP_REG, LOOP_BIT

				ENDM
					

READ_BYTE		MACRO	TEMP_REG
				
				READ_BITS TEMP_REG, #8

				ENDM
				
; reads byte to ACC and writes given byte in W_REG. destroys W_REG and TEMP_REG		
; 13 cycles per bit, 1 cycle setup time, 1 cycle result movement (external)
; -> 106 cycles / 8 bit -> fCPU >= 212 * fSCK  		
RW_BYTE			MACRO	TEMP_REG, W_REG				
				LOCAL	LOOP_SCK_LOW, LOOP_SCK_HIGH, LOOP_BIT
	
				MOV 	TEMP_REG, #8			; 1
				 
LOOP_BIT:		XCH		A, W_REG				; 1
				RLC		A						; 1		output << 1, MSB -> carry	
				MOV		MISO, C					; 2		MSB -> output
				L_SCK_LOW   					; 2 (*n)
													
				MOV		C, MOSI					; 1		read
				XCH		A, W_REG				; 1		save shifted output, restore input
				RLC		A						; 1		(input << 1) | read
				L_SCK_HIGH						; 2 (*n)
				DJNZ 	TEMP_REG, LOOP_BIT		; 2

				ENDM
				
; reads a byte without waiting for SCK going low at the end.
; this allows some calculation during the calculation of the last bit.		
; this is just READ_BYTE with an unrolled last loop!
; an average half bit costs 6,625 cycles. To be at least as fast as RW_BYTE,
; we have (4 cycles of last half bit + max half_bit - 4 for setting MISO)
; = 6,625 cycles -> 6 cycles to ensure that MSB of next byte is received
; correctly by master.
; as real systems master will have at least a small additional delay and this is
; the worst case calculation, we will be safe with 7 cycles.
; READ_BYTE needs two cycles less than RW_BYTE for first bit output -> there we
; have + 2  		
READ_BYTE_X		MACRO	TEMP_REG
				LOCAL	LOOP_SCK_LOW
	
				READ_BITS	TEMP_REG, #7
				
				CPL		MISO
				L_SCK_LOW						; we lose max. 2 cycles of high period
											
				MOV		C, MOSI					; 1
				RLC		A						; 1
				

				ENDM				
				
								

; ----------------------------------------------------------------------------
; unsigned long get4Bytes(void)
;
; reads four bytes over SPI. Works up to fSCK = fCPU/111 (see READ_BYTE for
; exact timing requirements).
; on return, SCK will be in idle state.
; ----------------------------------------------------------------------------
;                
; unsigned long get4Bytes(void)
				
				RSEG	?PR?get4Bytes?___SPI_ASM
				USING 0
				$REGUSE get4Bytes(A,R2,R4,R5,R6,R7)

get4Bytes:		
				READ_BYTE	R2
				MOV		R7, A		; R7 is LSB of return long
				READ_BYTE	R2
				MOV		R6, A
				READ_BYTE	R2
				MOV		R5, A
				READ_BYTE	R2
				MOV		R4, A		; R4 is MSB of return long			
				
				
g4b_finished:	RET			


; ----------------------------------------------------------------------------
; unsigned long get4Bytes_command(void)
;
; handles a few commands on its own.
; READ_BYTE -> returns data of given address in xdata memory in 4th SPI byte
; READ_BYTE_D -> returns data of given address in data memory in 4th SPI byte
; on return, SCK will be in idle state.
; ----------------------------------------------------------------------------
;                
; unsigned long get4Bytes(void)
				
				RSEG	?PR?get4Bytes_command?___SPI_ASM
				USING 0
				$REGUSE get4Bytes_command(A,R0,R2,R3,R4,R5,R6,R7,DPH,DPL)

get4Bytes_command:		
				READ_BYTE_X	R2
				MOV		R7, A			; COMMAND
				JBC		ACC.2, g4bc_read ; bit 0x04 set -> READ command
				L_SCK_HIGH				; wait for next bit
				READ_BYTE	R2
				MOV		R6, A
				READ_BYTE	R2
				MOV		R5, A
				READ_BYTE	R2
				MOV		R4, A			; R4 is MSB of return long			
				JMP		g4bc_finished
				
g4bc_read:		JZ		g4bc_xdata		; bit 0 clear -> read xdata
										; bit 0 set -> read data
				L_SCK_HIGH				; wait for next bit
				READ_BYTE	R2			; ADDR high
				MOV		R6, A			
				READ_BYTE_X	R2			; ADDR low
				MOV		R5, A			
				MOV		R0, A				
				MOV		A, @R0			; get data
				MOV		R3, A
				L_SCK_HIGH				; for slow SPI masters: wait for next bit
				RW_BYTE		R2, R3		; output data
				MOV		R4, A
				JMP		g4bc_finished
				 							


g4bc_xdata:
				L_SCK_HIGH				; wait for next bit
				READ_BYTE	R2			; ADDR high
				MOV		R6, A			
				MOV		DPH, A
				READ_BYTE_X	R2			; ADDR low		
				MOV		DPL, A				
				MOVX	A, @DPTR		; get data
				MOV		R3, A
				L_SCK_HIGH				; for slow SPI masters: wait for next bit
				RW_BYTE		R2, R3		; output data
				MOV		R4, A
				MOV		R5, DPL			; get ADDR low for correct return value
				;JMP		g4bc_finished
				
				
g4bc_finished:	RET				
				                
                
;------------------------------------------------------------------------------
; The END directive is ALWAYS required.
;------------------------------------------------------------------------------
                END             ; End Of File


