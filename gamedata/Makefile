ROMTOOL=../pc/romtools/romtool
MODEXTRACT=../pc/modextract/modextract
FMODPLAY=../pc/modextract/fmodPlay
USBASP_SPI=../pc/usbasp-spi/usbasp-spi
USBASP_SPI_OPTS=
XROM_START=32768
AUDIOPLAYER=aplay -f u8 -r 6510 -
MODFILE=mario.mod
LAYOUT=spaceinvaders.txt
OUTPUT=spaceinvaders.bin

all: rom

fmod: $(MODFILE)
	 $(MODEXTRACT) $(MODFILE) image/$(MODFILE)4_ image/$(MODFILE).pat 1 0101
	 $(MODEXTRACT) zeldag.mod image/zeldag4_ image/zeldag.mod.pat 1 051106120913

rom: $(LAYOUT) $(MODFILE) fmod
	$(ROMTOOL) $(LAYOUT) image/$(OUTPUT)

play: rom
	$(FMODPLAY) image/$(OUTPUT) 1 | $(AUDIOPLAYER)

program: rom
	$(USBASP_SPI) $(USBASP_SPI_OPTS) -o $(XROM_START) -b image/$(OUTPUT)
