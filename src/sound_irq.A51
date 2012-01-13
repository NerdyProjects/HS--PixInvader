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

;
$EJECT

EXTRN	DATA	(AS, ASIncr, ASIncrFracCnt, ASEnd, ASReload, ASVolume)
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
;           30.5 cycles average case (playing, no reload, worst case nibble/ avg case sign)
;			34.5 cycles "" for short samples, or high byte of SAMPLE END reached 
;                          (-> optimisation: lay SAMPLE END just behind a 256 byte boundary)
;           40 cycles worst case
;			VOLUME scaling adds 9-12 cycles, so we have 52 worst case!
;			Interrupt call/leave/sample output: 33 cycles (+ 4 when using MUL operation in ISR)
;			4 samples: 193 cycles total
;           with volume: 245 cycles total (worst case)
;           At 20 MHz, we would have ~210 CPU cycles (average) for each 2ms display interrupt.
;           This will definitively be way to less!
;           Without volume: we have ~890 cycles per display interrupt

PLAY_16B_SMP	MACRO	SAMPLE, INCR, INCR_FRAC, INCR_FRAC_CNT, SAMPLE_END, SAMPLE_RELOAD_OFFSET, RUN, NIBBLE, VOLUME
				LOCAL	off, no_reload_needed, low_nibble, finished_calc, sample_negative, sample_positive
				
				JNB RUN, off
			
				MOV		A, INCR_FRAC_CNT
				ADD		A, INCR_FRAC
				MOV		INCR_FRAC_CNT, A		; IncrFracCnt += IncrFrac
				MOV		A, SAMPLE+01H
				MOV		DPL, A				; store _old_ sample address, so we begin output at first sample point!
				ADDC	A, INCR
				MOV		R1, A			; Low address + Incr + IncrFracCnt Carry in R1
				
				MOV		A, SAMPLE
				MOV		DPH, A
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
				MOV		SAMPLE+01H, R1		; Save sample position
				MOV		SAMPLE, R0
				
				MOVX		A, @DPTR				; get sample & add to output
				
				JNB 	NIBBLE, low_nibble 
				SWAP	A
low_nibble:				
				ANL		A, #0FH				; only 4 bits of interest
				JNB		ACC.3, sample_positive	; audio sample positive
				ORL		A, #0F0H				; negative -> sign extend!		

sample_negative:
				; We would apply volume scaling _here_ (negative sample)
				;MOV		B, VOLUME
				;MUL		AB
				;SETB	C
				;RRC		A
				;SETB	C
				;RRC		A
				;JMP		finished_calc

sample_positive:
					
				; We would apply volume scaling _here_ (positive sample)
				;MOV		B, VOLUME
				;MUL		AB			; clears carry
				;RRC		A
				;CLR		C
				;RRC		A	
				
finished_calc:			    
				
				ADD		A, R7
				MOV		R7, A				
						
				
				
off:

				ENDM
				
				


;	Sample format: 4 bit signed integer (0000B = 0; 0111B = 7; 1111B = -1; 1000B = -8)
; just sign extend and add resulting sample points; convert to unsigned before output.
; because of 8 bit DAC we have no problem with just adding (4*(4 bit value) <= (6 bit value))
;
; byte format: <sample stream X> <sample stream Y> (high nibble (NibbleSelect=1) is X)         
;
int0_code_seg   SEGMENT CODE            ; segment for interrupt function
                RSEG    int0_code_seg   ; switch to this code segment
                USING   1               ; register bank for interrupt routine

; For timing calculation: A cycle is F_OSC / 12.
; At 6510.42 Hz we have an interrupt every 256 cycles.


timer0int:      PUSH    PSW			
                MOV     PSW,#08H        ; register bank 1
                PUSH    ACC
                PUSH	DPH
                PUSH	DPL
              	;PUSH	B
                MOV 	A, R7				; Sound sample output from previous run
                ADD		A, #128				; signed -> unsigned conversion for DAC
				MOV		DPTR, #SoundReg
				MOVX	@DPTR, A
				

				PLAY_16B_SMP	AS, ASIncr, ASIncr + 1, ASIncrFracCnt, ASEnd, ASReload, AS0R, AS0N, ASVolume                                            
				PLAY_16B_SMP	AS+2, ASIncr+2, ASIncr + 3, ASIncrFracCnt+1, ASEnd+1, ASReload+1, AS1R, AS1N, ASVolume + 1
				PLAY_16B_SMP	AS+4, ASIncr+4, ASIncr + 5, ASIncrFracCnt+2, ASEnd+2, ASReload+2, AS2R, AS2N, ASVolume + 2
				PLAY_16B_SMP	AS+6, ASIncr+6, ASIncr + 7, ASIncrFracCnt+3, ASEnd+3, ASReload+3, AS3R, AS3N, ASVolume + 3
				
				;POP		B
				POP		DPL
				POP		DPH
                POP     ACC
                POP     PSW
                RETI
                
;------------------------------------------------------------------------------
; The END directive is ALWAYS required.
;------------------------------------------------------------------------------
                END             ; End Of File


