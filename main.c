#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <SDL.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <curl/curl.h>

#define KEY_SIZE 128
#define URL_SIZE 256

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

void draw_text(SDL_Surface* surface, const char* text, TTF_Font* font, SDL_Color color, SDL_Rect* pos) {
    SDL_Surface* text_surface;

    if (!(text_surface = TTF_RenderText_Blended(font, text, color))) {
        printf("Failed to render text to surface! SDL_ttf error: %s\n", TTF_GetError());
    }
    else {
        SDL_BlitSurface(text_surface, NULL, surface, pos);
        SDL_FreeSurface(text_surface);
    }
}

SDL_Surface* load_image(const char* path) {
    SDL_Surface* image = IMG_Load(path);
    if (!image) {
        printf("SDL_image: Failed to load image at %s. %s\n", path, IMG_GetError());
        exit(2);
    }
    return image;
}

void read_key(char* filename, char* key, int len) {
    FILE* key_file = fopen(filename, "r");
    if (!key_file) {
        printf("Failed to open key file %s!", filename);
        exit(2);
    }
    fread(key, sizeof *key, len, key_file);
    
    // Remove trailing newlines
    key[strcspn(key, "\r\n ")] = '\0';

    fclose(key_file);
}

int main() {
    // Read key from file
    char api_key[KEY_SIZE];
    read_key("key.txt", api_key, KEY_SIZE);

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

        // Initialize SDL IMAGE
        int img_flags = IMG_INIT_PNG;
        int img_initted = IMG_Init(img_flags);
        if ((img_initted&img_flags) != img_flags) {
            printf("SDL_image failed to initialize png!");
            printf("SDL_image error: %s\n", IMG_GetError());
            return 1;
        }

        surface = SDL_GetWindowSurface(window);

        // Get weather data
        char url[URL_SIZE];
        char cityname[] = "Chicago";
        int len = sprintf(url, "api.openweathermap.org/data/2.5/weather?q=%s&appid=%s", cityname, api_key);
        
        printf("url: %s, %d\n", url, (int) strlen(url));

        CURL* curl;
        CURLcode res;

        curl_global_init(CURL_GLOBAL_DEFAULT);

        curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url);

            res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                printf("curl_easy_perform failed: %s\n", curl_easy_strerror(res));
            }
            curl_easy_cleanup(curl);
        }

        curl_global_cleanup();
        
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

        // Load image
        SDL_Surface* weather_image = load_image("assets/iconfinder_Raining.png");
        SDL_BlitSurface(weather_image, NULL, surface, NULL);

        // Load font
        TTF_Font* font = TTF_OpenFont("assets/OpenSans-Regular.ttf", 16);
        if (!font) {
          printf("Could not load font! SDL_ttf error: %s\n", TTF_GetError());
          return 1;
        }
        
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

            // Clear surface
            SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0x00, 0x00, 0x00));

            // Render
            SDL_BlitSurface(weather_image, NULL, surface, NULL);

            SDL_FillRect(surface, &player.rect, player.color);

            // Draw text
            SDL_Color text_color = {0xFF, 0xFF, 0xFF, 0xFF};
            SDL_Rect text_position = {100, 10, 0, 0};
            draw_text(surface, "Hello, World!", font, text_color, &text_position);

            SDL_UpdateWindowSurface(window);
        }

        // Free sound
        Mix_FreeChunk(gSound);
        gSound = NULL;

        // Free music
        Mix_FreeMusic(gMusic);
        gMusic = NULL;

        // Free font
        TTF_CloseFont(font);
        font = NULL;
        
        SDL_FreeSurface(weather_image);
        SDL_DestroyWindow(window);
        
        TTF_Quit();
        IMG_Quit();
        Mix_Quit();
        SDL_Quit();
    }

    return 0;
}
