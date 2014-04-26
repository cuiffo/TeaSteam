GCC= gcc
FLAGS= -Wall
LIBS= -lncurses

games: obj/connectFour.o
	mkdir -p bin; \
	$(GCC) $(FLAGS) -o bin/connectFour obj/connectFour.o $(LIBS)

obj/connectFour.o: src/connectFour.c
	mkdir -p obj; \
	$(GCC) $(FLAGS) -c -o obj/connectFour.o src/connectFour.c $(LIBS)

clean:
	rm obj/* bin/*
