main: main.c
	gcc -o main main.c -lm `sdl2-config --cflags --libs` -lSDL2_mixer
