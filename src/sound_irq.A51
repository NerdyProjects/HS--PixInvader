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

; For timing calculation: A cycle is F_OSC / 12.
; At 7812.5 Hz we have an interrupt every 256 cycles.
; Timing: Preamble + Jump -> up to 5 cycles
;         Before first channel: 15 cycles
;		  After last channel: 10
;         -> unvoidable 30 cycles
; Timing per 8 bit sample index channel: 
;         When channel is not playing: 2
;	      When channel is playing (aufeinanderfolgende) samplepoints: 27 (-1 for low nibble samples)
;         When channel has to reload (worst case): +4 (or +1 when samplelength=256 and reload at end)
;
; 4x 8 bit sample index channel + unavoidable:
;  30 + 4*26 = 30 + 108 = 134 samples (best case)
;  30 + 4*31 = 30 + 124 = 154 samples (worst case)
;  average case: mostly 136, sometimes 154 (every ~20 samples avg) -> avg 137-138 cycles
; this calculation does not include any sample-turn-off when loop is disabled (sample just continues
; to play same samplepoint over and over again -> generates just DC offset which is filtered at output)


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
				JNB		ACC.3, stream0_finished_calc	; audio sample positive
				ORL		A, F0H				; negative -> sign extend!

stream0_finished_calc:
				ADD 	A, R7				; Add to output sample
				MOV		R7, A				; and move to output sample storage
								
stream0_off:

				; 16 bit indexed stream with resample capability
				
; EXTRN	DATA	(AS, ASIncr, ASIncrFrac, ASIncrFracCnt, ASEnd, ASReload, ASReadIdx, ASLoopFrom, ASEndAt)

;			2 cycles: channel off
;			21 cycles: no reload
;			25 cycles: no reload, but highest end position byte matches
;			30 cycles: reload
				JNB AS4R, stream4_off
			
				MOV		A, ASIncrFracCnt+04H
				ADD		A, ASIncrFrac+04H
				MOV		ASIncrFracCnt+04H, A
				MOV		A, AS+2*(04H)+01H
				ADDC	A, #0
				MOV		R1, A			; Low address in R1
				
				MOV		A, AS+2*(04H)
				ADCC	A, #0
				
				MOV		R0, A			; High address in R0
				
				; Carry is clear because we do not overflow over 64k (is this true??) todo
				SUBB	A, ASEnd+2*(00H)		; subtract high end from high position
				JNZ		stream4_no_reload_needed
				; we will never increment a complete high byte, so there is no overflow checking
				; neccessary. When A is not Zero, we always have a few points more to play
				
				; Carry is always clear because of this never-overflow-condition
				
				MOV		A, R1
				SUBB	A, ASEnd+2*(00H)+01H	; subtract low end position
				JC		stream4_no_reload_needed
				; carry set when we subtract more than we have (->end position has to be reached)
				ADD		A, ASReload+2*(00H)+1	; Add low repeat position
				MOV		R1, A
				MOV		A, ASReload+2*(00H)
				ADDC	A, #0
				MOV		R0, A
				
stream4_no_reload_needed:
				MOV		DPH, R0
				MOV		DPL, R1
				MOV		AS+2*(04H)+01H, R1		; Save sample position
				MOV		AS+2*(04H), R0
				
				MOV		A, @DPTR				; get sample & add to output
				ADD		A, R7
				MOV		R7, A									
				
				
stream4_off:								


				
				POP		DPL
				POP		DPH
                POP     ACC
                POP     PSW
                RETI
                
;------------------------------------------------------------------------------
; The END directive is ALWAYS required.
;------------------------------------------------------------------------------
                END             ; End Of File


