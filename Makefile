CC=cc -O2 -Wall -Wextra `sdl2-config --cflags`
LIB=`sdl2-config --libs`
OBJ=fx16.o
BIN=fx16

default: $(OBJ)
	$(CC) -o $(BIN) $(OBJ) $(LIB)

clean:
	rm -f $(BIN) $(OBJ)
