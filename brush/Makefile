.phony: all clean

view.out: view.o brush.o
	clang++ view.o brush.o -g -o view.out -lm -lGL -lGLU -lglut

brush.o: brush.cpp brush.hpp
	clang++ -c brush.cpp -g -o brush.o

view.o: view.cpp brush.hpp
	clang++ -c view.cpp -g -o view.o

clean:
	-@rm *.o *.out