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

char weather_map[8][40] = {
        "assets/iconfinder_Lightning_Cloudy.png",
        "assets/iconfinder_Drip.png",
        "assets/iconfinder_Raining.png",
        "assets/iconfinder_Snow_Cloudy.png",
        "assets/iconfinder_Foggy.png",
        "assets/iconfinder_Sunny.png",
        "assets/iconfinder_Moon.png",
        "assets/iconfinder_Cloudy.png"
    };  

double kelvin_to_celsius(double kelvin) {
    return kelvin - 273.15;
}

double kelvin_to_fahrenheit(double kelvin) {
    return ((kelvin_to_celsius(kelvin) * 9) / 5) + 32;
}

int weather_id_to_array_id(int weather_id) {
    if (weather_id < 200) {
        return -1;
    }
    if (weather_id < 300) {
        return 0;
    }
    if (weather_id < 500) {
        return 1;
    }
    if (weather_id < 600) {
        return 2;
    }
    if (weather_id < 700) {
        return 3;
    }
    if (weather_id < 800) {
        return 4;
    }
    if (weather_id == 800) {
        return 5;
    }
    if (weather_id > 800) {
        return 7;
    }
    return -1;
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
    
    // Remove trailing newline
    key[strcspn(key, "\r\n ")] = '\0';

    fclose(key_file);
}

void read_json(char* json_string, int len, double* temp, int* weather_id) {
    json_tokener* tok = json_tokener_new();

    json_object* jobj = NULL;
    enum json_tokener_error jerr;

    // Parse json response
    jobj = json_tokener_parse_ex(tok, json_string, len);
    jerr = json_tokener_get_error(tok);
    if (jerr != json_tokener_success) {
        printf("Json parse failed! %s", json_tokener_error_desc(jerr));
        exit(2);
    }

    // Get the value of /main/temp
    json_object* t = NULL;
    json_object_object_get_ex(jobj, "main", &t);
    json_object_object_get_ex(t, "temp", &t);
    if (json_object_is_type(t, json_type_double)) {
        *temp = json_object_get_double(t);
    }
    else {
        *temp = 0;
    }

    // Get value of /weather/0/id
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

    // Clean up json object
    json_object_put(jobj);
}

void call_api(char* api_key, char* cityname, double* kelvin, int* weather_id) {
    char url[URL_SIZE];
    int len = sprintf(url, "api.openweathermap.org/data/2.5/weather?q=%s&appid=%s", cityname, api_key);
    
    printf("url: %s, %d\n", url, len);

    struct response json;

    json.text = (char*)malloc(1*sizeof(char));
    *json.text = ' ';
    json.size = 1;

    curl_global_init(CURL_GLOBAL_DEFAULT);

    CURL* curl;
    CURLcode res;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&json);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform failed: %s\n", curl_easy_strerror(res));
        }
        curl_easy_cleanup(curl);
    }
 
    curl_global_cleanup();
    
    printf("Response text: %s\nResponse size: %d\n", json.text, (int) json.size);

    read_json(json.text, json.size, kelvin, weather_id);

    printf("Temperature and weather id: %f %d\n", *kelvin, *weather_id);

    free(json.text);
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
        window = SDL_CreateWindow("MyWeather", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 256, 256, SDL_WINDOW_SHOWN);

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

        // Get window's surface
        surface = SDL_GetWindowSurface(window);

        // Get weather data
        double temperature = 0.0;
        int weather_id = 0;
        call_api(api_key, "Chicago", &temperature, &weather_id);

        // Load image and display it
        SDL_Surface* weather_image = load_image(weather_map[weather_id_to_array_id(weather_id)]);
        SDL_BlitSurface(weather_image, NULL, surface, NULL);
        SDL_UpdateWindowSurface(window);

        // Load font
        TTF_Font* font = TTF_OpenFont("assets/OpenSans-Regular.ttf", 40);
        if (!font) {
          printf("Could not load font! SDL_ttf error: %s\n", TTF_GetError());
          return 1;
        }

        SDL_Event event;

        // Load music
        Mix_Music * gMusic = Mix_LoadMUS("assets/music.wav");
        if (gMusic == NULL) {
            printf("Failed to load music! SDL_mixer error: %s\n", Mix_GetError());
            return 1;
        }

        // Play music
        Mix_VolumeMusic(SDL_MIX_MAXVOLUME);
        Mix_PlayMusic(gMusic, -1);

        char degree_text[20];
        sprintf(degree_text, "%.1f C", kelvin_to_celsius(temperature));

        bool quit = false;
        bool celsius = true;
        while (!quit) {
            // Input
            while(SDL_PollEvent(&event) != 0) {
                switch (event.type) {
                    case SDL_QUIT:
                        quit = true;
                        break;
                    case SDL_MOUSEBUTTONDOWN:
                        celsius = !celsius;
                        if (celsius) {
                            sprintf(degree_text, "%.1f C", kelvin_to_celsius(temperature));
                        }
                        else {
                            sprintf(degree_text, "%.0f F", kelvin_to_fahrenheit(temperature));
                        }
                        break;
                    case SDL_KEYDOWN:
                        if (event.key.keysym.sym == SDLK_ESCAPE) {
                            quit = true;
                        }
                        break;
                }
            }

            // Clear surface
            SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0x00, 0x00, 0x00));

            // Render
            SDL_BlitSurface(weather_image, NULL, surface, NULL);

            // Draw text
            SDL_Color text_color = {0xFF, 0xFF, 0xFF, 0xFF};
            SDL_Rect text_position = {100, 10, 0, 0};
            draw_text(surface, degree_text, font, text_color, &text_position);

            SDL_UpdateWindowSurface(window);
        }

        // Free music
        Mix_FreeMusic(gMusic);
        gMusic = NULL;

        // Free font
        TTF_CloseFont(font);
        font = NULL;
        
        // Free up SDL window and it's surface
        SDL_FreeSurface(weather_image);
        SDL_DestroyWindow(window);
        
        // Cleanup SDL library and it's extensions
        TTF_Quit();
        IMG_Quit();
        Mix_Quit();
        SDL_Quit();
    }

    return 0;
}
