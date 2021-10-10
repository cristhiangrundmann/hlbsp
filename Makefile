.phony: all view

all: view

test.out: test.cpp brush.cpp brush.h hlbsp.h
	g++ test.cpp brush.cpp -g -o test.out

view: view.out
	./view.out maps/c1a0.bsp

view.out: view.o brush.o
	g++ view.o brush.o -O3 -o view.out -lm -lGL -lGLU -lglut

brush.o: brush.cpp brush.h hlbsp.h
	g++ brush.cpp -c -O3 -o brush.o

view.o: view.cpp brush.h hlbsp.h
	g++ view.cpp -c -O3 -o view.o

clean:
	-rm *.o *.out