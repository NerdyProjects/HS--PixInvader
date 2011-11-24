
SDCC_OPTS=--model-large --xram-size 8192 --stack-size 128
SDCC=sdcc
SRC=display.c sound.c
TARGET=main
OBJECTS=$(SRC:.c=.rel)


%.rel: ../%.c ../%.h
	sdcc $(SDCC_OPTS) -c $<

sdcc_compile: $(OBJECTS)
	sdcc $(SDCC_OPTS) ../$(TARGET).c $(OBJECTS)

clean:
	-rm $(OBJECTS)
	-rm $(OBJECTS:.rel=.asm)
	-rm $(OBJECTS:.rel=.lst)
	-rm $(OBJECTS:.rel=.rst)
	-rm $(OBJECTS:.rel=.sym)
	-rm $(TARGET).{lnk,asm,lst,rst,sym,rel,map,mem}
	-rm main.ihx
