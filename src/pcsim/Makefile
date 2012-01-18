TARGET=sim
SRC=main.c ../pixinvaders.c ../font.c


compile: $(SRC)
	gcc $(SRC) -o $(TARGET) -D_NO_8051_INCLUDE -D_DEBUG -lncurses -lpthread

clean:
	-rm $(TARGET)
