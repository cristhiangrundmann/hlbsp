.phony: all view hlbsp

all: hlbsp.out view.out

hlbsp.out: hlbsp.o brush.o 
	g++ hlbsp.o brush.o -O3 -o hlbsp.out

view.out: view.o brush.o
	g++ view.o brush.o -O3 -o view.out -lm -lGL -lGLU -lglut

hlbsp.o: hlbsp.cpp brush.h hlbsp.h
	g++ hlbsp.cpp -c -O3 -o hlbsp.o

brush.o: brush.cpp brush.h hlbsp.h
	g++ brush.cpp -c -O3 -o brush.o

view.o: view.cpp brush.h hlbsp.h
	g++ view.cpp -c -O3 -o view.o

clean:
	-rm *.o *.out