all: main.cc glhelper.o timer.o vector.o keyboard.o
	g++ main.cc -g -o bin/main lib/glhelper.o lib/timer.o lib/vector.o lib/keyboard.o assimp-3.1.1/lib/libassimp.so.3.1.1 `sdl-config --cflags --libs` -lGL -lGLEW -lSDL_image

glhelper.o:
	g++ -c src/glhelper.cc -o lib/glhelper.o

timer.o:
	g++ -c src/timer.cc -o lib/timer.o

vector.o:
	g++ -c src/vector.cc -o lib/vector.o

keyboard.o:
	g++ -c src/keyboard.cc -o lib/keyboard.o

clean:
	@rm -f *~ src/*~ lib/* bin/*
