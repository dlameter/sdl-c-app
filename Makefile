main: main.c
	gcc -o main main.c -lm `sdl2-config --cflags --libs` `curl-config --cflags --libs` `pkg-config --cflags json-c --libs json-c` -lSDL2_mixer -lSDL2_ttf -lSDL2_image -Wall
clean:
	rm main
