MY_PATH=build_keil7
KEIL_PATH=../../../C51
BIN_PATH=$(KEIL_PATH)/BIN
INC_PATH=$(KEIL_PATH)/INC
LIB_PATH=$(KEIL_PATH)/LIB
CC=wine $(BIN_PATH)/c51.exe
CC_OPTS="LARGE OMF2 PREPRINT SYMBOLS INCDIR(..\\) OBJECTADVANCED CODE"
LD=wine $(BIN_PATH)/LX51.exe
LD_OPTS="CLASSES(XDATA(X:0x0000-X:0x001FFF)) CODE"
ASM=wine $(BIN_PATH)/A51.exe
ASM_OPTS="DB"
HEX=wine $(BIN_PATH)/Ohx51.exe
SRC=main.c display.c sound.c
ASRC=sound_irq.A51
OUTPUT=PIXINVADERS
OBJECTS=$(SRC:.c=.obj) $(ASRC:.A51=.abj)
comma:=,
empty:=
space:= $(empty) $(empty)


%.obj: ../%.c ../%.h
	-$(CC) $< $(CC_OPTS) "pr ("$(*F)".lst) oj("$@")"

%.abj: ../%.A51
	$(ASM) $< $(ASM_OPTS) "pr ("$(*F)".lst) oj("$@")"

all: hex

compile: $(OBJECTS)
	$(LD) $(subst $(space),$(comma),$(OBJECTS)) TO $(OUTPUT).abs $(LD_OPTS)

hex: compile
	$(HEX) $(OUTPUT).ABS 

clean:
#	-rm $(OBJECTS)
	-rm $(OBJECTS:.obj=.lst) $(OBJECTS:.abj=.lst)
	-rm $(OUTPUT).{ABS,COD,HTM,SBR,MAP}
	-rm $(OUTPUT).hex
