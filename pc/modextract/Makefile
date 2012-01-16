all: rom

rom: layout.txt
	../romtools/a.out layout.txt meskalin.img

modextract: modextract.c
	gcc -o modextract modextract.c
	
fmodplay: fmodPlay.c
	gcc -o fmodPlay fmodPlay.c
