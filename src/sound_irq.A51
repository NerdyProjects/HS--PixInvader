;------------------------------------------------------------------------------
; Source code template for A251/A51 assembler modules.
; Copyright (c) 1995-2000 KEIL Software, Inc.
;------------------------------------------------------------------------------
$NOMOD51                ; disable predefined 8051 registers
#include <atmel/regx52.h>      // include CPU definition file (for example, 8052)

;------------------------------------------------------------------------------
; Module name (optional)
;------------------------------------------------------------------------------
NAME            SOUND_ISR

SILENT_SAMPLE	 EQU	07H
;
$EJECT

EXTRN	DATA	(AS, ASIncr, ASIncrFrac, ASIncrFracCnt, ASEnd, ASReload, ASReadIdx, ASLoopFrom, ASEndAt)
EXTRN	BIT		(AS0N, AS0R)                                                                
EXTRN	XDATA	(SoundReg)

;------------------------------------------------------------------------------
; To include an interrupt service routins, provide an LJMP to the ISR at the
; interrupt vector address.
;------------------------------------------------------------------------------
                CSEG    AT  0BH         ; 0BH is address for Timer 0 interrupt
                LJMP    timer0int
;
;	Sample format: 4 bit signed integer (0000B = 0; 0111B = 7; 1111B = -1; 1000B = -8)
;   8 bit acc gives: 1111B + 0001B = 10000B (-1 + 1 = 16 ~ 0)
;					10000B + 1111B = 11111B (16 +-1 = 31 ~ -1)
;					 0111B + 0111B =  1110B ( 7 + 7 = -2 ~ 14)
;	Sample format: 4 bit unsigned integer (0000B = 0; 0111B = 7; 1111B = 15)
;	8 bit acc gives: 1111B + 0001B = 10000B (15 + 1 = 16) 
; because of 6 bit DAC we have no problem with this one. static mixing of 4
; channels makes it easier: always add 7 for a turned off channel.
; no overflow handling neccessary with unsigned format.       
;
; byte format: <sample stream X> <sample stream Y> (high nibble (NibbleSelect=1) is X)         
;
int0_code_seg   SEGMENT CODE            ; segment for interrupt function
                RSEG    int0_code_seg   ; switch to this code segment
                USING   1               ; register bank for interrupt routine

timer0int:      PUSH    PSW
                MOV     PSW,#08H        ; register bank 1
                PUSH    ACC
                PUSH	DPH
                PUSH	DPL
              
                MOV 	A, R7				; Sound sample output from previous run
				MOV		DPTR, #SoundReg
				MOVX	@DPTR, A
				
				
				; 8 bit indexed stream
				JNB		AS0R, stream0_off
				
				MOV		A, ASIncrFracCnt
				ADD		A, ASIncrFrac
				MOV		ASIncrFracCnt, A
				MOV		A, ASReadIdx
				ADDC	A, ASIncr			; ReadIdx += Incr + (carry of frac-addition)
											; Carry set or Idx> EndAt: reload (or stop)
											; reload has to be done with substraction of (end - start)
				MOV 	R0, A				; save new index val
				JC		stream0_reload_needed	; (1: carry set), if carry clear, we have to check for ASEndAt

				SUBB	A, ASEndAt			; (carry is clear)
				JNC		stream0_no_reload_needed	; (2: idx <= endAt -> no reload)
				JC		stream0_reload_needed_substraction_done

stream0_reload_needed:				
				CLR C
				SUBB	A, ASEndAt
stream0_reload_needed_substraction_done:
				ADD		A, ASLoopFrom
				MOV		R0, A

stream0_no_reload_needed:
				MOV 	A, R0				; get saved index val				
				MOV		ASReadIdx, A		; save new index to global
				ADD		A, AS+1				; low address + index
				MOV		DPL, A
				MOV		DPH, AS				; high address
				MOV		A, @DPTR
				JNB 	AS0N, stream0_low_nibble 
				SWAP	A
stream0_low_nibble:				
				ANL		A, 0FH				; only 4 bits of interest
				JNB		ACC.3, stream0_finished	; audio sample positive
				ORL		A, F0H				; negative -> sign extend!

stream0_finished:
				ADD 	A, R7
				MOV		R7, A					
								
stream0_off:


				
				POP		DPL
				POP		DPH
                POP     ACC
                POP     PSW
                RETI
                
;------------------------------------------------------------------------------
; The END directive is ALWAYS required.
;------------------------------------------------------------------------------
                END             ; End Of File


