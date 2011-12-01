;------------------------------------------------------------------------------
; Source code template for A251/A51 assembler modules.
; Copyright (c) 1995-2000 KEIL Software, Inc.
;------------------------------------------------------------------------------
$NOMOD51                ; disable predefined 8051 registers
#include <atmel/regx52.h>      // include CPU definition file (for example, 8052)

;------------------------------------------------------------------------------
; Change names in lowercase to suit your needs.
;
; This assembly template gives you an idea of how to use the A251/A51
; Assembler.  You are not required to build each module this way-this is only
; an example.
;
; All entries except the END statement at the End Of File are optional. 
;
; If you use this template, make sure you remove any unused segment declarations,
; as well as unused variable space and assembly instructions.
;
; This file cannot provide for every possible use of the A251/A51 Assembler.
; Refer to the A51/A251 User's Guide for more information.
;------------------------------------------------------------------------------

;------------------------------------------------------------------------------
; Module name (optional)
;------------------------------------------------------------------------------
NAME            SOUND_ISR

;------------------------------------------------------------------------------
; Here, you may import symbols form other modules.
;------------------------------------------------------------------------------
;EXTRN   CODE   (code_symbol)    ; May be a subroutine entry declared in
                                ; CODE segments or with CODE directive.
;
;
;EXTRN   DATA   (data_symbol)    ; May be any symbol declared in DATA segments
                                ; segments or with DATA directive.

EXTRN 	DATA 	(AudioStream, AudioStreamEnd, AudioStreamIncrement, AudioStreamIncrementFrac, AudioStreamIncrementVal)                                

;EXTRN   BIT    (bit_symbol)     ; May be any symbol declared in BIT segments
                                ; or with BIT directive.

;EXTRN   XDATA  (xdata_symbol)   ; May be any symbol declared in XDATA segments
                                ; or with XDATA directive.
EXTRN	XDATA	(SoundReg)

;EXTRN   NUMBER (typeless_symbol); May be any symbol declared with EQU or SET
                                ; directive

;------------------------------------------------------------------------------
; You may include more than one symbol in an EXTRN statement.
;------------------------------------------------------------------------------
;EXTRN   CODE (sub_routine1, sub_routine2), DATA (variable_1)

;------------------------------------------------------------------------------
; Force a page break in the listing file.
;------------------------------------------------------------------------------
$EJECT

;------------------------------------------------------------------------------
; Here, you may export symbols to other modules. 
;------------------------------------------------------------------------------

;------------------------------------------------------------------------------
; You may include more than one symbol in a PUBLIC statement.
;------------------------------------------------------------------------------

;------------------------------------------------------------------------------
; Put the STACK segment in the main module.
;------------------------------------------------------------------------------
;?STACK          SEGMENT IDATA           ; ?STACK goes into IDATA RAM.
;                RSEG    ?STACK          ; switch to ?STACK segment.
;                DS      5               ; reserve your stack space
                                        ; 5 bytes in this example.


$EJECT

;------------------------------------------------------------------------------
; Put segment and variable declarations here.
;------------------------------------------------------------------------------

;------------------------------------------------------------------------------
; DATA SEGMENT--Reserves space in DATA RAM--Delete this segment if not used.
;------------------------------------------------------------------------------
;data_seg_name   SEGMENT DATA            ; segment for DATA RAM.
;                RSEG    data_seg_name   ; switch to this data segment
;data_variable:  DS      1               ; reserve 1 Bytes for data_variable
;data_variable1: DS      2               ; reserve 2 Bytes for data_variable1

;------------------------------------------------------------------------------
; XDATA SEGMENT--Reserves space in XDATA RAM--Delete this segment if not used.
;------------------------------------------------------------------------------
;xdata_seg_name  SEGMENT XDATA           ; segment for XDATA RAM
;                RSEG    xdata_seg_name  ; switch to this xdata segment
;xdata_variable: DS      1               ; reserve 1 Bytes for xdata_variable
;xdata_array:    DS      500             ; reserve 500 Bytes for xdata_array

;------------------------------------------------------------------------------
; INPAGE XDATA SEGMENT--Reserves space in XDATA RAM page (page size: 256 Bytes)
; INPAGE segments are useful for @R0 addressing methodes.
; Delete this segment if not used.
;------------------------------------------------------------------------------
;page_xdata_seg  SEGMENT XDATA INPAGE    ; INPAGE segment for XDATA RAM
;                RSEG    xdata_seg_name  ; switch to this xdata segment
;xdata_variable1:DS      1               ; reserve 1 Bytes for xdata_variable1

;------------------------------------------------------------------------------
; ABSOLUTE XDATA SEGMENT--Reserves space in XDATA RAM at absolute addresses.
; ABSOLUTE segments are useful for memory mapped I/O.
; Delete this segment if not used.
;------------------------------------------------------------------------------
;                XSEG    AT 8000H        ; switch absolute XDATA segment @ 8000H
;XIO:            DS      1               ; reserve 1 Bytes for XIO port
;XCONFIG:        DS      1               ; reserve 1 Bytes for XCONFIG port

;------------------------------------------------------------------------------
; BIT SEGMENT--Reserves space in BIT RAM--Delete segment if not used.
;------------------------------------------------------------------------------
bit_seg_name    SEGMENT BIT             ; segment for BIT RAM.
                RSEG    bit_seg_name    ; switch to this bit segment
SOUND_NIBBLE_LOW:   DBIT    1               ; reserve 1 Bit for bit_variable
SOUND_NL_1:		DBIT	1
SOUND_PLAY_1:	DBIT	1
;bit_variable1:  DBIT    4               ; reserve 4 Bits for bit_variable1

;------------------------------------------------------------------------------
; Add constant (typeless) numbers here.
;------------------------------------------------------------------------------
;typeless_number EQU     0DH                     ; assign 0D hex
;typeless_num1   EQU     typeless_number-8       ; evaluate typeless_num1
;
$EJECT

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
; byte format: <high nibble> <low nibble>. Low nibble is played first!           
;
;------------------------------------------------------------------------------
; Give each interrupt function its own code segment.
;------------------------------------------------------------------------------
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
				
; TIMING: worst case (high nibble) 26, case (low nibble) 19; (-4 on 255/256 because of end detection)
; 11 at stop (25/18/12 possible when A=#07H before JZ __STREAM0_SKIP)
; KEIL worst case 33 (high nibble) 24 (low nibble); (-4 on 255/256 because of end detection)  

; --------------PROCESS STREAM 0 --------------               
                MOV		A, AudioStream+01H
                MOV		DPL, A				; faster load of DPL
                XRL		A, AudioStreamEnd+01H
                JNZ		__STREAM0_PLAY		; Low Byte differs -> not at stop
                MOV		A, AudioStream
                XRL		A, AudioStreamEnd
                JZ		__STREAM0_SKIP		; Low & High Byte equal -> at stop position
                
	__STREAM0_PLAY:
				MOV		DPH, AudioStream
                MOVX	A, @DPTR			; load sound sample
                JB		SOUND_NL_1, __STREAM0_NL
                SWAP	A
                INC 	DPTR				; after high nibble we need next byte
                MOV		AudioStream+01H, DPL	; now is a good time to increment/store
                MOV		AudioStream, DPH		; (INC AudioStream, mov -> A, JNZ, INC AS+1 is faster but we need to safe A for that)

				                
	__STREAM0_NL:     
				ANL		A, #0FH
				JMP		__STREAM0_SAVE
				
	__STREAM0_SKIP:
				MOV		A, #07H
	
	__STREAM0_SAVE:
				           
                MOV		R7, A				; R7: sample buffer
                							; next channels need additional ADD		A, R7
                							
	__STREAM0_END:        
	
	; if play bit set:
	; Play sample from *AudioStream (todo!!)
	; Increment by AudioStreamIncrement
	; Increment AudioStreamIncrementVal by AudioStreamIncrementFrac
	; Increment AudioStream with carry
	; if AudioStream == AudioStreamEnd -> a) disable play, b) reload AudioStream with loop start
	; a) clear play bit
	; b) AudioStream = AudioStreamLoopStart
	;
	;in: r7 - sample buffer (add our sample there) (+ our state vars)
	;out: r7 - sample buffer (+ our state vars)
	; --------------PROCESS STREAM 1 --------------    
				JB		SOUND_PLAY_1, __STREAM1_SKIP
				MOV		A, AudioStream+03H
				MOV		DPL, A
				MOV		A, AudioStream+02H
				MOV		DPH, A
				
				MOV		A, AudioStreamIncrementVal
				ADD		A, AudioStreamIncrementFrac
				MOV		AudioStreamIncrementVal, A		; AudioStreamIncrementVal += ASIFrac; Carry 9th bit
				
				CLR		A								; What a waste...
				ADDC	A, AudioStreamIncrement+01H		; Incrementation + frac part of incrementation
														; carry should be cleared by this (for sane increments)
				
				RRC		A								; A /= 2, Carry set: nibble change needed
				JNC		__STREAM1_NO_NIBBLE_CHANGE
				CPL		SOUND_NL_1
				JNB		SOUND_NL_1, __STREAM1_NIBBLE_CHANGED_NO_INCREMENT
				ADD		A, #1
				
			__STREAM1_NIBBLE_CHANGED_NO_INCREMENT:
		
			__STREAM1_NO_NIBBLE_CHANGE:
				MOV		R0, A
				MOV		A, DPL
				ADD		A, R0
				MOV		A, DPH
				ADDC	A, #0			; missing: store new pointer, compare with stream end, get audio data; end comparison with nibble check neccessary!
				
				
				
			__STREAM1_SKIP:
				MOV		A, #07H
				ADD		A, R7
				MOV		R7, A
				
				
				           
;                MOV		A, AudioStream+03H
;                MOV		DPL, A				; faster load of DPL
;                XRL		A, AudioStreamEnd+03H
;                JNZ		__STREAM1_PLAY		; Low Byte differs -> not at stop
;                MOV		A, AudioStream+02H
;                XRL		A, AudioStreamEnd+02H
;                JZ		__STREAM1_SKIP		; Low & High Byte equal -> at stop position
                
;	__STREAM1_PLAY:
;				MOV		DPH, AudioStream+02H
;                MOVX	A, @DPTR			; load sound sample
;                JB		SOUND_NIBBLE_LOW, __STREAM1_NL
;                SWAP	A
;                INC 	DPTR				; after high nibble we need next byte
;                MOV		AudioStream+03H, DPL	; now is a good time to increment/store
;                MOV		AudioStream+02H, DPH		; (INC AudioStream, mov -> A, JNZ, INC AS+1 is faster but we need to safe A for that)

				                
;	__STREAM1_NL:     
;				ANL		A, #0FH
;				JMP		__STREAM1_SAVE
				
;	__STREAM1_SKIP:
;				MOV		A, #07H
	
;	__STREAM1_SAVE:
;				ADD		A, R7           
;                MOV		R7, A				; R7: sample buffer
                							
                							
	__STREAM1_END:          
	
	; --------------PROCESS STREAM 2 --------------               
                MOV		A, AudioStream+05H
                MOV		DPL, A				; faster load of DPL
                XRL		A, AudioStreamEnd+05H
                JNZ		__STREAM2_PLAY		; Low Byte differs -> not at stop
                MOV		A, AudioStream+04H
                XRL		A, AudioStreamEnd+04H
                JZ		__STREAM2_SKIP		; Low & High Byte equal -> at stop position
                
	__STREAM2_PLAY:
				MOV		DPH, AudioStream+04H
                MOVX	A, @DPTR			; load sound sample
                JB		SOUND_NIBBLE_LOW, __STREAM2_NL
                SWAP	A
                INC 	DPTR				; after high nibble we need next byte
                MOV		AudioStream+05H, DPL	; now is a good time to increment/store
                MOV		AudioStream+04H, DPH		; (INC AudioStream, mov -> A, JNZ, INC AS+1 is faster but we need to safe A for that)

				                
	__STREAM2_NL:     
				ANL		A, #0FH
				JMP		__STREAM2_SAVE
				
	__STREAM2_SKIP:
				MOV		A, #07H
	
	__STREAM2_SAVE:
				ADD		A, R7           
                MOV		R7, A				; R7: sample buffer
                							
                							
	__STREAM2_END:          
	
	
	; --------------PROCESS STREAM 3 --------------               
                MOV		A, AudioStream+07H
                MOV		DPL, A				; faster load of DPL
                XRL		A, AudioStreamEnd+07H
                JNZ		__STREAM3_PLAY		; Low Byte differs -> not at stop
                MOV		A, AudioStream+06H
                XRL		A, AudioStreamEnd+06H
                JZ		__STREAM3_SKIP		; Low & High Byte equal -> at stop position
                
	__STREAM3_PLAY:
				MOV		DPH, AudioStream+06H
                MOVX	A, @DPTR			; load sound sample
                JB		SOUND_NIBBLE_LOW, __STREAM3_NL
                SWAP	A
                INC 	DPTR				; after high nibble we need next byte
                MOV		AudioStream+07H, DPL	; now is a good time to increment/store
                MOV		AudioStream+06H, DPH		; (INC AudioStream, mov -> A, JNZ, INC AS+1 is faster but we need to safe A for that)

				                
	__STREAM3_NL:     
				ANL		A, #0FH
				JMP		__STREAM3_SAVE
				
	__STREAM3_SKIP:
				MOV		A, #07H
	
	__STREAM3_SAVE:
				ADD		A, R7           
                MOV		R7, A				; R7: sample buffer
                							
                							
	__STREAM3_END:                  							
				; output of sound will be handled at start of next interrupt (less jitter)
				CPL		SOUND_NIBBLE_LOW	; next nibble
				
				POP		DPL
				POP		DPH
                POP     ACC
                POP     PSW
                RETI
                
;------------------------------------------------------------------------------
; The END directive is ALWAYS required.
;------------------------------------------------------------------------------
                END             ; End Of File


