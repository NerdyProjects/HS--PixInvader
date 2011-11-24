
SDCC_OPTS=--model-large --xram-size 8192 --stack-size 128
SDCC=sdcc
SRC=display.c sound.c
OBJECTS=$(SRC:.c=.rel)


%.rel: ../%.c ../%.h
	sdcc $(SDCC_OPTS) -c $<

sdcc_compile: $(OBJECTS)
	sdcc $(SDCC_OPTS) ../main.c $(OBJECTS)

clean:
	rm $(OBJECTS)
	rm main.ihx
