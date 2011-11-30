.PHONY: sdcc keil sim

all: sdcc keil sim

sdcc:
	make -C build_sdcc

keil:
	make -C build_keil7

sim:
	make -C pcsim


clean:
	make -C build_sdcc clean
	make -C build_keil7 clean
	make -C pcsim clean
