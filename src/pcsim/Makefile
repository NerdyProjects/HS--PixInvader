TARGET=sim
SRC=main.c ../pixinvaders.c


compile: $(SRC)
	gcc $(SRC) -o $(TARGET) -D_NO_8051_INCLUDE -lncurses -lpthread

clean:
	rm $(TARGET)
