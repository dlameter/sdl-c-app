#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <SDL.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <curl/curl.h>
#include <json-c/json.h>

#define KEY_SIZE 128
#define URL_SIZE 256

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

void read_json(char* json_string, int len, double* temp, int* weather_id) {
    json_tokener* tok = json_tokener_new();

    json_object* jobj = NULL;
    enum json_tokener_error jerr;


    jobj = json_tokener_parse_ex(tok, json_string, len);
    jerr = json_tokener_get_error(tok);
    if (jerr != json_tokener_success) {
        printf("Json parse failed! %s", json_tokener_error_desc(jerr));
        exit(2);
    }


    json_object* t = NULL;
    json_object_object_get_ex(jobj, "main", &t);
    json_object_object_get_ex(t, "temp", &t);
    if (json_object_is_type(t, json_type_double)) {
        *temp = json_object_get_double(t);
    }
    else {
        *temp = 0;
    }


    json_object* wid = NULL;
    json_object_object_get_ex(jobj, "weather", &wid);
    wid = json_object_array_get_idx(wid, 0);
    json_object_object_get_ex(wid, "id", &wid);
    if (json_object_is_type(wid, json_type_int)) {
        *weather_id = json_object_get_int(wid);
    }
    else {
        *weather_id = -1;
    }

    json_object_put(jobj);
}

struct response {
    char* text;
    size_t size;
};

size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t true_size = size * nmemb;

    struct response* temp = (struct response *) userdata;

    char* data = realloc(temp->text, temp->size + true_size + 1);
    if (ptr == NULL) {
        return 0;
    }

    temp->text = data;
    memcpy(&(temp->text[temp->size]), ptr, true_size);
    temp->size += true_size;
    temp->text[temp->size] = 0;

    return true_size;
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
        
        printf("url: %s, %d\n", url, len);

        struct response json;
        json.text = (char*)malloc(1*sizeof(char));
        CURL* curl;
        CURLcode res;

        curl_global_init(CURL_GLOBAL_DEFAULT);

        curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url);

            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &write_callback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&json);

            res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                printf("curl_easy_perform failed: %s\n", curl_easy_strerror(res));
            }
            curl_easy_cleanup(curl);
        }

        curl_global_cleanup();
        
        printf("Response text: %s\nResponse size: %d\n", json.text, (int) json.size);

        double temperature = 0.0;
        int weather_id = 0;
        read_json(json.text, json.size, &temperature, &weather_id);

        printf("Temperature and weather id: %f %d\n", temperature, weather_id);

        free(json.text);
        
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

            // Reset timer
            start_time = SDL_GetTicks();

            // Clear surface
            SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0x00, 0x00, 0x00));

            // Render
            SDL_BlitSurface(weather_image, NULL, surface, NULL);

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
