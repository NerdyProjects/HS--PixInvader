;------------------------------------------------------------------------------
; Source code template for A251/A51 assembler modules.
; Copyright (c) 1995-2000 KEIL Software, Inc.
;------------------------------------------------------------------------------
$NOMOD51                ; disable predefined 8051 registers
#include <atmel/regx52.h>      // include CPU definition file (for example, 8052)

;------------------------------------------------------------------------------
; PixInvader sound routines
; Sound ISR: Play up to 4 channels. see corresponding C file for configuration.
;            takes about half of available CPU cycles when all channels playing.
;            uses timer0
; playSound function:  Fast ASM version of supplied playSound C function.
;                      Has some alignment requirements for sound data structure.
;                      see corresponding C files and comments for details.
;           
; 2011/2012 by Matthias Larisch
;------------------------------------------------------------------------------

;------------------------------------------------------------------------------
; Module name (optional)
;------------------------------------------------------------------------------
NAME            SOUND_ISR

;
$EJECT

EXTRN	CODE	(PeriodTable)
EXTRN	DATA	(AS, ASIncr, ASIncrFracCnt, ASEnd, ASReload, ASVolume)
EXTRN	BIT		(AS0N, AS0R, AS1N, AS1R, AS2N, AS2R, AS3N, AS3R)                                                                
EXTRN	XDATA	(SoundReg, SampleInfo)

?PR?_playSample?___SOUND_ASM             SEGMENT CODE	

PUBLIC		_playSample			


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
;           42 cycles worst case
;			VOLUME scaling adds 9-12 cycles, so we have 52 worst case!
;			Interrupt call/leave/sample output: 33 cycles (+ 4 when using MUL operation in ISR)
;			4 samples: 193 cycles total
;           with volume: 245 cycles total (worst case)
;           At 20 MHz, we would have ~210 CPU cycles (average) for each 2ms display interrupt.
;           Only ~143 at worst case. This will definitively be way to less as SONG playing
;			needs about 500-1000 cycles (-> may work, but lots of display jitter)
;           Without volume: we have ~820 cycles per display interrupt
;
; Parameters:
; SAMPLE - unsigned char xdata * to next playing sample point
; INCR - unsigned short with incremental value for resampling - sample += INCR/256
; INCR_FRAC_CNT - unsigned char for applying 1/256 fractional part for resample
; SAMPLE_END - unsigned char xdata * to one byte after the sample. This will not be played.
; SAMPLE_RELOAD_OFFSET - unsigned char xdata * to the sample point we should enter when looping.
;                        a value <= 0x00FF will disable sample looping
; RUN - bit telling us the channel is playing or not (will be set to 0 after finishing a non looped sample)
; NIBBLE - bit for selecting the nibble where the sample is stored.
;          sample data is always stored in one byte per sample point, but we have the data for two 
;          _independent_ samples in every byte. NIBBLE = 1 selects the high nibble, 0 the lower one.

PLAY_16B_SMP	MACRO	SAMPLE, INCR, INCR_FRAC, INCR_FRAC_CNT, SAMPLE_END, SAMPLE_RELOAD_OFFSET, RUN, NIBBLE, VOLUME
				LOCAL	off, no_reload_needed, low_nibble, finished_calc, sample_negative, sample_positive, no_sample_turn_off
				
				JNB RUN, off
			
				MOV		A, INCR_FRAC_CNT
				ADD		A, INCR_FRAC
				MOV		INCR_FRAC_CNT, A	; IncrFracCnt += IncrFrac
				MOV		A, SAMPLE+01H
				MOV		DPL, A				; store _old_ sample address, so we begin output at first sample point!
				ADDC	A, INCR
				MOV		R1, A				; Low address + Incr + IncrFracCnt Carry in R1
				
				MOV		A, SAMPLE
				MOV		DPH, A
				ADDC	A, #0			
				
				MOV		R0, A				; High address (+ carry of lAddr-Addition) in R0
				
				; Carry is clear because we should not overflow over 64k (this is guaranted to be true
				; when sound sample data is not at the end of data rom)
				SUBB	A, SAMPLE_END		; subtract high end from high position
				JNZ		no_reload_needed
				; we never increment a complete high byte, so there is no overflow checking
				; neccessary. When A is not zero, we always have a few points more to play
				
				; Carry is always clear here. No borrow is needed when the result is zero and
				; we know we subtracted something < 256
				
				MOV		A, R1
				SUBB	A, SAMPLE_END + 01H		; subtract low end position
				JC		no_reload_needed
				; carry set when we subtract more than we have (->end position _not_ reached now)
				
				ADD		A, SAMPLE_RELOAD_OFFSET + 01H	; Add low repeat position
				MOV		R1, A
				MOV		A, SAMPLE_RELOAD_OFFSET			; high repeat position 
				JNZ		no_sample_turn_off
				CLR		RUN						; disable sample repetition on ZERO high reload byte!
												; this enforces samples with repetition to be above 0x00FF
												; in address space.
												; this is accepted as there should always be enough other data
												; that can be stored in the first page of address space...
												; as most samples will be played from ROM (>= 0x8000) there
												; will never be a problem.
no_sample_turn_off:				
				ADDC	A, #0					; Add carry from repetition-pointer-addition			
				MOV		R0, A					; Next sample position in R0:R1
				
no_reload_needed:
				MOV		SAMPLE+01H, R1		; Save sample position
				MOV		SAMPLE, R0
				
				MOVX		A, @DPTR				; get last sample
													; address was put in DPTR some lines above
				
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
				
				; volume scaling has a some bits left:
				; 4 channels with 4 bit -> 6 bit used max.
				; 4 bit multiplication -> 10 bit used max.
				; so we have to divide by 2 bits afterwards ->
				; 8 bit max. output. We have an 8 bit DAC.

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
				
				


; Sample format: 4 bit signed integer (0000B = 0; 0111B = 7; 1111B = -1; 1000B = -8)
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
                MOV     PSW,#08H        	; register bank 1
                PUSH    ACC
                PUSH	DPH
                PUSH	DPL
              	;PUSH	B					; would be needed for volume scaling
              	MOV		A, #0				; clear sample buffer
                XCH 	A, R7				; Sound sample output from previous run
                ADD		A, #128				; signed -> unsigned conversion for DAC
				MOV		DPTR, #SoundReg
				MOVX	@DPTR, A
				

				PLAY_16B_SMP	AS, ASIncr, ASIncr + 1, ASIncrFracCnt, ASEnd, ASReload, AS0R, AS0N, ASVolume                                            
				PLAY_16B_SMP	AS+2, ASIncr+2, ASIncr + 3, ASIncrFracCnt+1, ASEnd+2, ASReload+2, AS1R, AS1N, ASVolume + 1
				PLAY_16B_SMP	AS+4, ASIncr+4, ASIncr + 5, ASIncrFracCnt+2, ASEnd+4, ASReload+4, AS2R, AS2N, ASVolume + 2
				PLAY_16B_SMP	AS+6, ASIncr+6, ASIncr + 7, ASIncrFracCnt+3, ASEnd+6, ASReload+6, AS3R, AS3N, ASVolume + 3
				
				;POP		B				; see above (volume)
				POP		DPL
				POP		DPH
                POP     ACC
                POP     PSW
                RETI
                
;------------------------------------------------------------------------------
; void playSample(unsigned char idx, unsigned char channel, unsigned char period)
; 				                R7                 R5                     R3
;
; SampleInfo: Array of structs of:
; xdata-ptr to sample
; xdata-ptr behind sample
; byte nibble
; xdata-ptr to sample loop entry
; (2+2+1+2 -> 7 byte)
; timing: 80 cycles worst case for ch4 (some less for lower channels)
; alignment requirement for SampleInfo struct:
; must not cross 256 byte boundary!
;------------------------------------------------------------------------------

; DPTR must be preloaded to SampleInfo of sample to setup!
; R2:R3 must be preloaded to sample period value!
; Parameters have same meaning as ISR parameters.
SETUP_SAMPLE	MACRO	SAMPLE, INCR, INCR_FRAC_CNT, SAMPLE_END, SAMPLE_RELOAD, RUN, NIBBLE

				CLR		RUN							; stop channel
				
				MOV		INCR, R2					; set period value (increments)
				MOV		INCR+1, R3
				
				MOVX	A, @DPTR
				MOV		SAMPLE, A
				INC		DPTR
				MOVX	A, @DPTR
				MOV		SAMPLE+1, A					; Sample pointer loaded.
				INC		DPTR						; DPTR = &SampleInfo[smp].length	
				
				MOVX	A, @DPTR
				MOV		SAMPLE_END, A
				INC		DPTR
				MOVX	A, @DPTR					; Sample (end+1) pointer loaded.
				MOV		SAMPLE_END+1, A				; DPTR = &SampleInfo[smp].nibble
				INC		DPTR
				
				MOVX	A, @DPTR
				MOV		C, ACC.0
				MOV		NIBBLE, C				; NIBBLE loaded.
				INC		DPTR						; DPTR = &SampleInfo[smp].loopEntry
				
				MOVX	A, @DPTR
				MOV		SAMPLE_RELOAD, A
				INC		DPTR
				MOVX	A, @DPTR					; Sample reload pointer loaded.
				MOV		SAMPLE_RELOAD+1, A
				
				CLR		A
				MOV		INCR_FRAC_CNT, A	
				
				SETB	RUN							; start channel		
				
				ENDM																	
																	

				
				


SI_SIZE			EQU		7
             	RSEG	?PR?_playSample?___SOUND_ASM
				USING 0		; called from display interrupt and main program. DO NOT USE ABSOLUTE REGISTERS!
				

_playSample:			  
				MOV		A, R3						; Load period values to R3:R2 at first.
				ADD		A, ACC
				ADD		A, #LOW PeriodTable
				MOV		DPL, A
				CLR		A
				ADDC	A, #HIGH PeriodTable
				MOV		DPH, A
				CLR		A
				MOVC	A,@A+DPTR
				MOV		R2, A						; Incr to R2
				MOV		A, #1
				MOVC	A,@A+DPTR
				MOV		R3, A						; IncrFrac to R3
 
				MOV		B, R7						; calculate &SampleInfo[idx]
				MOV		A, #SI_SIZE
				MUL		AB
				MOV		DPH, #HIGH SampleInfo		; completely in one 256 byte page
				ADD		A, #LOW SampleInfo
				MOV		DPL, A						; &SampleInfo[idx] -> DPTR 
				MOV		A, R5
				JNZ		testCh1
				SETUP_SAMPLE	AS, ASIncr, ASIncrFracCnt, ASEnd, ASReload, AS0R, AS0N
				JMP _playSample_fin
				
testCh1:		DEC		A
				JNZ		testCh2
				SETUP_SAMPLE	AS+2, ASIncr+2, ASIncrFracCnt+1, ASEnd+2, ASReload+2, AS1R, AS1N
				JMP 	_playSample_fin
				
testCh2:		DEC		A
				JNZ		testCh3
				SETUP_SAMPLE	AS+4, ASIncr+4, ASIncrFracCnt+2, ASEnd+4, ASReload+4, AS2R, AS2N
				JMP 	_playSample_fin
				
testCh3:		DEC		A
				JNZ		_playSample_fin
				SETUP_SAMPLE	AS+6, ASIncr+6, ASIncrFracCnt+3, ASEnd+6, ASReload+6, AS3R, AS3N
				
_playSample_fin:												
				
				RET	
				
                
;------------------------------------------------------------------------------
; The END directive is ALWAYS required.
;------------------------------------------------------------------------------
                END             ; End Of File


