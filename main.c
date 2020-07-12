#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <SDL.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>

struct Player {
    double x;
    double y;
    double speed;
    SDL_Rect rect;
    Uint32 color;
};

struct ButtonState {
    bool up;
    bool down;
    bool right;
    bool left;
};

int main() {
    SDL_Window* window = NULL;
    SDL_Surface* surface = NULL;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        printf("SDL_Init failed: %s\n", SDL_GetError());

        SDL_Quit();

        return 1;
    }
    else {
        window = SDL_CreateWindow("MyWeather", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 600, 400, SDL_WINDOW_SHOWN);

        if (window == NULL) {
            printf("Window creation failed: %s\n", SDL_GetError());
            
            SDL_DestroyWindow(window);
            SDL_Quit();
            
            return 1;
        }

        // Initialize SDL MIX
        if ( Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 1, 2048) < 0) {
            printf("SDL_mixer could not initialize! SDL_mixer error: %s\n", Mix_GetError());
            return 1;
        }

        // Initialize SDL TTF
        if ( TTF_Init() == -1 ) {
            printf("SDL_ttf could not initialize! SDL_ttf error: %s\n", TTF_GetError());
            return 1;
        }

        surface = SDL_GetWindowSurface(window);

        // Build player
        struct Player player;
        player.x = 50.0;
        player.y = 50.0;
        player.color = SDL_MapRGB(surface->format, 0xFF, 0x00, 0x00);
        player.speed = 10;
        
        SDL_Rect rect;
        rect.x = (int) player.x;
        rect.y = (int) player.y;
        rect.w = 20;
        rect.h = 30;

        player.rect = rect;
        printf("rect x = %d y = %d\n", player.rect.x, player.rect.y);

        // Build button state holder
        struct ButtonState button;
        button.up = false;
        button.down = false;
        button.right = false;
        button.left = false;

        // Load background image
        SDL_Surface* image = SDL_LoadBMP("another_test.bmp");
        SDL_BlitSurface(image, NULL, surface, NULL);

        // Load font
        
        SDL_FillRect(surface, &player.rect, player.color);

        SDL_UpdateWindowSurface(window);

        SDL_Event event;

        // Load music
        Mix_Music * gMusic = Mix_LoadMUS("assets/music.wav");
        if (gMusic == NULL) {
            printf("Failed to load music! SDL_mixer error: %s\n", Mix_GetError());
            return 1;
        }
        
        // Load sound
        Mix_Chunk * gSound = Mix_LoadWAV("assets/baseball_hit.wav");
        if (gSound == NULL) {
            printf("Failed to load baseball_hit! SDL_mixer error: %s\n", Mix_GetError());
            return 1;
        }

        Mix_VolumeMusic(SDL_MIX_MAXVOLUME);
        // Play music
        Mix_PlayMusic(gMusic, -1);

        // Create Timer
        Uint32 start_time = SDL_GetTicks();

        bool quit = false;
        while (!quit) {
            double y_speed = 0;
            double x_speed = 0;

            // Input
            while(SDL_PollEvent(&event) != 0) {
                switch (event.type) {
                    case SDL_QUIT:
                    case SDL_MOUSEBUTTONDOWN:
                        quit = true;
                        break;
                    case SDL_KEYDOWN:
                        if (event.key.keysym.sym == SDLK_UP) {
                            button.up = true;
                        }
                        if (event.key.keysym.sym == SDLK_DOWN) {
                            button.down = true;
                        }
                        if (event.key.keysym.sym == SDLK_LEFT) {
                            button.right = true;
                        }
                        if (event.key.keysym.sym == SDLK_RIGHT) {
                            button.left = true;
                        }
                        if (event.key.keysym.sym == SDLK_1) {
                            Mix_PlayChannel(-1 , gSound, 0);
                        }
                        break;
                    case SDL_KEYUP:
                        if (event.key.keysym.sym == SDLK_UP) {
                            button.up = false;
                        }
                        if (event.key.keysym.sym == SDLK_DOWN) {
                            button.down = false;
                        }
                        if (event.key.keysym.sym == SDLK_LEFT) {
                            button.right = false;
                        }
                        if (event.key.keysym.sym == SDLK_RIGHT) {
                            button.left = false;
                        }
                        break;
                }
            }

            // Apply keys pressed to player direction
            if (button.up) {
                y_speed -= 1;
            }
            if (button.down) {
                y_speed += 1;
            }
            if (button.left) {
                x_speed += 1;
            }
            if (button.right) {
                x_speed -= 1;
            }

            if (y_speed != 0 || x_speed != 0) {
                // Normalize speed vector
                double square_root = sqrt(y_speed * y_speed + x_speed * x_speed);
                y_speed = (y_speed / square_root);
                x_speed = (x_speed / square_root);

                // Move player
                double delta_time = (SDL_GetTicks() - start_time) / 1000.f;
                player.x += x_speed * player.speed * delta_time;
                player.y += y_speed * player.speed * delta_time;
                printf("player x = %f y = %f\n", player.x, player.y);

                player.rect.x = (int) player.x;
                player.rect.y = (int) player.y;
                printf("rect x = %d y = %d\n", player.rect.x, player.rect.y);
            }

            // Reset timer
            start_time = SDL_GetTicks();

            // Render
            SDL_BlitSurface(image, NULL, surface, NULL);

            SDL_FillRect(surface, &player.rect, player.color);

            SDL_UpdateWindowSurface(window);
        }

        // Free sound
        Mix_FreeChunk(gSound);
        gSound = NULL;

        // Free music
        Mix_FreeMusic(gMusic);
        gMusic = NULL;
        
        SDL_FreeSurface(image);
        SDL_DestroyWindow(window);
        
        TTF_Quit();
        Mix_Quit();
        SDL_Quit();
    }

    return 0;
}
