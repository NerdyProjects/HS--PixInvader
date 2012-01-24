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
	
; read in given number of bits (#1-#255 or even register/data are allowed)
; and return it in accumulator (therefore > #8 gives us only the 8 LSBs)
; TEMP_REG is used as counter and will be set to zero after this.
READ_BITS		MACRO	TEMP_REG, BITS
				LOCAL	LOOP_BIT
	
				MOV 	TEMP_REG, BITS			;    2 cycles setup time + 
LOOP_BIT:		CPL		MISO					;    2 cycles result movement
				L_SCK_LOW   					;    8 cycles per bit needed
												;    1 cycle per bit for output complement
				MOV		C, MOSI					;    -> 74 cycles / 8 bit
				RLC		A						;    -> fCPU has to be >= 111 * fSCK
				L_SCK_HIGH
				DJNZ 	TEMP_REG, LOOP_BIT

				ENDM
					

; read in 8 bits and return in ACC. TEMP_REG will be set to zero.
READ_BYTE		MACRO	TEMP_REG
				
				READ_BITS TEMP_REG, #8

				ENDM
				
				
RW_BITS			MACRO	TEMP_REG, BITS		
				LOCAL	LOOP_BIT
	
				MOV 	TEMP_REG, BITS			; 1
				 
LOOP_BIT:		RLC		A						; 1		IN/OUTPUT << 1, MSB -> carry	
				MOV		MISO, C					; 2		MSB -> output
				L_SCK_LOW   					; 2 (*n)		
				MOV		C, MOSI					; 1		read
				L_SCK_HIGH						; 2 (*n)
				DJNZ 	TEMP_REG, LOOP_BIT		; 2
				RLC		A						; LSB -> IN/OUTPUT
				ENDM
						
; reads byte to ACC and writes given byte in ACC. destroys TEMP_REG		
; 13 cycles per bit, 1 cycle setup time, 1 cycle result movement (external)
; -> 106 cycles / 8 bit -> fCPU >= 212 * fSCK  	
; *todo* cycle calculation is wrong as we saved about 2 cycles per loop 	
RW_BYTE			MACRO	TEMP_REG		
				
				RW_BITS	TEMP_REG, #8
				
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
	
				READ_BITS	TEMP_REG, #7
				
				CPL		MISO
				L_SCK_LOW						; we lose max. 2 cycles of high period
											
				MOV		C, MOSI					; 1
				RLC		A						; 1
				
				ENDM				
				

; reads/writes a byte without waiting for SCK going low at end.
; similar to READ_BYTE_X				
RW_BYTE_X		MACRO	TEMP_REG

				RW_BITS		TEMP_REG, #7
				MOV		MISO, C					; 2		MSB -> output
				L_SCK_LOW   					; 2 (*n)		
				MOV		C, MOSI
				RLC		A
				
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
				SETB	MISO		; will be CPLd before first
								; bit -> send 0b01010101
				READ_BYTE	R2		; (0x55)
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
; READ_MULTI -> returns requested number of data bytes in additional
;  READ_MULTI_DATA requests (3 bytes at a time)
; will set lowest byte of return value to OP_ERROR on error!
; on return, SCK will be in idle state.
; ----------------------------------------------------------------------------
;                
; unsigned long get4Bytes_command(void)
				
				RSEG	?PR?get4Bytes_command?___SPI_ASM
				USING 0
				$REGUSE get4Bytes_command(A,R0,R2,R3,R4,R5,R6,R7,DPH,DPL)

get4Bytes_command:		
				SETB	MISO			; send 0x55 by toggling
				READ_BYTE_X	R2
				MOV		R7, A			; COMMAND
				JBC		ACC.2, g4bc_read ; bit 0x04 set -> READ command
				JBC		ACC.3, g4bc_r_block ; bit 0x08 set -> READ MULTI command
				L_SCK_HIGH				; wait for next bit
				READ_BYTE	R2
				MOV		R6, A
				READ_BYTE	R2
				MOV		R5, A
				READ_BYTE	R2
				MOV		R4, A			; R4 is MSB of return long			
				JMP		g4bc_finished
				
g4bc_r_block:
				L_SCK_HIGH
				READ_BYTE	R2
				MOV		DPH, A
				READ_BYTE	R2
				MOV		DPL, A
				READ_BYTE	R2
				MOV		R4, A			; data count (in bytes)
				JMP		g4bc_r_block_l	; conditional jump cannot reach whole code
										; so split this one here.
				
							
				
g4bc_read:		JZ		g4bc_xdata		; bit 0 clear -> read xdata
										; bit 0 set -> read data
				L_SCK_HIGH				; wait for next bit
				READ_BYTE	R2			; ADDR high
				MOV		R6, A			
				READ_BYTE_X	R2			; ADDR low
				MOV		R5, A			
				MOV		R0, A				
				MOV		A, @R0			; get data
				L_SCK_HIGH				; for slow SPI masters: wait for next bit
				RW_BYTE		R2			; output data
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
				L_SCK_HIGH				; for slow SPI masters: wait for next bit
				RW_BYTE		R2			; output data
				MOV		R4, A
				MOV		R5, DPL			; get ADDR low for correct return value
				JMP		g4bc_finished				
				
				
g4bc_r_block_l:	READ_BYTE_X	R2
				ADD		A, #(256-OP_MULTI_DATA)
				JNZ		g4bc_error		; we want OP_MULTI_DATA packets!
				
				MOVX	A, @DPTR
				L_SCK_HIGH
				RW_BYTE_X	R2
				
				INC		DPTR
				MOVX	A, @DPTR
				RW_BYTE_X	R2
				
				INC		DPTR
				MOVX	A, @DPTR
				RW_BYTE_X	R2
				
				MOV		A, R4
				ADD		A, #(256-3)	; subtract 3 from data counter
				MOV		R4, A
				JC		g4bc_r_block_l	; overflow? -> still more to substract. loop!
				
				JMP		get4Bytes_command	; await next command			
								
g4bc_error:
				MOV		R7, #OP_ERROR
				;JMP		g4bc_finished				 							

				
				
g4bc_finished:	RET				
				                
                
;------------------------------------------------------------------------------
; The END directive is ALWAYS required.
;------------------------------------------------------------------------------
                END             ; End Of File


