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

EXTRN	DATA	(AS, ASIncr, ASIncrFrac, ASIncrFracCnt, ASEnd, ASReload)
EXTRN	BIT		(AS0N, AS0R, AS1N, AS1R, AS2N, AS2R, AS3N, AS3R)                                                                
EXTRN	XDATA	(SoundReg)

;------------------------------------------------------------------------------
; To include an interrupt service routins, provide an LJMP to the ISR at the
; interrupt vector address.
;------------------------------------------------------------------------------
                CSEG    AT  0BH         ; 0BH is address for Timer 0 interrupt
                LJMP    timer0int
; MACRO DEFINITION

; EXTRN	DATA	(AS, ASIncr, ASIncrFrac, ASIncrFracCnt, ASEnd, ASReload, ASReadIdx, ASLoopFrom, ASEndAt)

; 16 bit indexed stream with resample capability
;			2 cycles: channel off
;			21 cycles: no reload
;			25 cycles: no reload, but highest end position byte matches
;			30 cycles: reload
;          +7 cycles for nibble/sign handling
;          -> about 37 cycles worst case, 30 cycles average case.
;          -> max. 6 channels possible

PLAY_16B_SMP	MACRO	SAMPLE, INCR, INCR_FRAC, INCR_FRAC_CNT, SAMPLE_END, SAMPLE_RELOAD_OFFSET, RUN, NIBBLE
				LOCAL	off, no_reload_needed, low_nibble, finished_calc
				
				JNB RUN, off
			
				MOV		A, INCR_FRAC_CNT
				ADD		A, INCR_FRAC
				MOV		INCR_FRAC_CNT, A		; IncrFracCnt += IncrFrac
				MOV		A, SAMPLE+01H
				ADDC	A, INCR
				MOV		R1, A			; Low address + Incr + IncrFracCnt Carry in R1
				
				MOV		A, SAMPLE
				ADDC	A, #0			
				
				MOV		R0, A			; High address (+ carry of lAddr-Addition) in R0
				
				; Carry is clear because we should not overflow over 64k (this is guaranted to be true
				; when sound sample data is not at the end of data rom)
				SUBB	A, SAMPLE_END		; subtract high end from high position
				JNZ		no_reload_needed
				; we (hope to) never increment a complete high byte, so there is no overflow checking
				; neccessary. When A is not zero, we always have a few points more to play
				
				; Carry is always clear here. No borrow is needed when the result is zero and
				; we know we subtracted something < 256
				
				MOV		A, R1
				SUBB	A, SAMPLE_END + 01H	; subtract low end position
				JC		no_reload_needed
				; carry set when we subtract more than we have (->end position has to be reached)
				
				ADD		A, SAMPLE_RELOAD_OFFSET + 01H	; Add low repeat position
				MOV		R1, A
				MOV		A, SAMPLE_RELOAD_OFFSET		; high repeat position is given or 
				ADDC	A, #0					;  given+1 (by carry)
				MOV		R0, A
				
no_reload_needed:
				MOV		DPH, R0
				MOV		DPL, R1
				MOV		SAMPLE+01H, R1		; Save sample position
				MOV		SAMPLE, R0
				
				MOVX		A, @DPTR				; get sample & add to output
				; no reload: 24 cycles till here
				
				JNB 	NIBBLE, low_nibble 
				SWAP	A
low_nibble:				
				ANL		A, #0FH				; only 4 bits of interest
				JNB		ACC.3, finished_calc	; audio sample positive
				ORL		A, #0F0H				; negative -> sign extend!		

finished_calc:					
					
				ADD		A, R7
				MOV		R7, A				
				
				; play/add: worst case (high nibble, negative): 9
				;           best case (low nibble, positive): 7				
				
				
off:

				ENDM
				
				


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
				

				PLAY_16B_SMP	AS, ASIncr, ASIncrFrac, ASIncrFracCnt, ASEnd, ASReload, AS0R, AS0N                                            
				PLAY_16B_SMP	AS+2, ASIncr+1, ASIncrFrac+1, ASIncrFracCnt+1, ASEnd+1, ASReload+1, AS1R, AS1N
				PLAY_16B_SMP	AS+4, ASIncr+2, ASIncrFrac+2, ASIncrFracCnt+2, ASEnd+2, ASReload+2, AS2R, AS2N
				PLAY_16B_SMP	AS+6, ASIncr+3, ASIncrFrac+3, ASIncrFracCnt+3, ASEnd+3, ASReload+3, AS3R, AS3N
				
				
				POP		DPL
				POP		DPH
                POP     ACC
                POP     PSW
                RETI
                
;------------------------------------------------------------------------------
; The END directive is ALWAYS required.
;------------------------------------------------------------------------------
                END             ; End Of File


