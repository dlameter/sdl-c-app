main: main.c
	gcc -o main main.c -lm `sdl2-config --cflags --libs` `curl-config --cflags --libs` -lSDL2_mixer -lSDL2_ttf -lSDL2_image
clean:
	rm main
