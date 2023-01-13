#include <stdio.h>
#include <stdbool.h>
#include <argp.h>
#include <getopt.h>
#include <SDL2/SDL.h>
#include <unistd.h>

#include "chip8.h"

/* CLI PARSING STUFF */
void fprint_usage(FILE* stream, int exit_code)
{
    fprintf(stream, "Usage: ch8 [OPTIONS...] [ROM]\n");
    fprintf(stream, 
            "   -h  --help                 Display this usage message.\n"
            "   -d  --delay     MILLIS     Delay between clock cycles in milliseconds.\n"
            "   -s  --scale     FACTOR     Integer scale factor for video.\n"
            "   -C  --on-color  0xRRGGBBAA Color of ON pixels.\n"
            "   -c  --off-color 0xRRGGBBAA Color of OFF pixels.\n");
    exit(exit_code);
}

struct config
{
    char*    rom;
    int      delay_ms;
    int      scale_factor;
    uint32_t color_on;
    uint32_t color_off;
};

void args_parse(struct config* config, int argc, char** argv)
{
    const struct option long_options[] = {
        {"help",      no_argument,       NULL, 'h'},
        {"delay",     required_argument, NULL, 'd'},
        {"scale",     required_argument, NULL, 's'},
        {"on-color",  required_argument, NULL, 'C'},
        {"off-color", required_argument, NULL, 'c'},
        {NULL, 0, NULL, 0}
    };
    const char* short_options = "hd:s:C:c:";
    
    int next_option;
    
    do {
        next_option = getopt_long(argc, argv, short_options, long_options, NULL);
        
        switch (next_option) {
        case 'h':
            fprint_usage(stdout, EXIT_SUCCESS);
        
        case 'd':
            config->delay_ms = strtol(optarg, NULL, 10);
            if (config->delay_ms <= 0) {
                printf("Invalid value for delay (must be positive integer).\n");
                fprint_usage(stderr, EXIT_FAILURE);
            }
            break;
        case 's':
            config->scale_factor = strtol(optarg, NULL, 10);
            if (config->scale_factor <= 0) {
                printf("Invalid value for scale factor (must be positive integer).\n");
                fprint_usage(stderr, EXIT_FAILURE);
            }
            break;
        case 'C':
            config->color_on = strtol(optarg, NULL, 16);
            if (config->color_on <= 0) {
                printf("Invalid value for ON color (format: 0xRRGGBBAA).\n");
                fprint_usage(stderr, EXIT_FAILURE);
            }
            break;
        case 'c':
            config->color_off = strtol(optarg, NULL, 16);
            if (config->delay_ms <= 0) {
                printf("Invalid value for OFF color (format: 0xRRGGBBAA).\n");
                fprint_usage(stderr, EXIT_FAILURE);
            }
            break;
        case '?':
            fprint_usage(stderr, EXIT_FAILURE);
        case -1:
            break;
        default:
            //this is not supposed to happen lol
            fprintf(stderr, "If you're seeing this, tell me how you got this message.\n");
            fprint_usage(stderr, EXIT_FAILURE);
        }
    } while (next_option != -1);
    
    //optind now points to first non-option argument, i.e. the ROM
    config->rom = argv[optind];
}
/* CLI PARSING END */

int keymap[16][2]  = {
    {SDLK_1, 0x1}, {SDLK_2, 0x2}, {SDLK_3, 0x3}, {SDLK_4, 0xC},
    {SDLK_q, 0x4}, {SDLK_w, 0x5}, {SDLK_e, 0x6}, {SDLK_r, 0xD},
    {SDLK_a, 0x7}, {SDLK_s, 0x8}, {SDLK_d, 0x9}, {SDLK_f, 0xE},
    {SDLK_z, 0xA}, {SDLK_x, 0x0}, {SDLK_c, 0xB}, {SDLK_v, 0xF}
};

bool handle_input(struct Chip8* chip8)
{
    bool quit = false;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch(event.type) {
            case SDL_QUIT:
                quit = true;
                break;
            
            case SDL_KEYDOWN:
                for (int i = 0; i < 16; i++) {
                    if (event.key.keysym.sym == keymap[i][0]) {
                        chip8->keys[keymap[i][1]] = 1;
                    }
                }
                break;
                
            case SDL_KEYUP:
                for (int i = 0; i < 16; i++) {
                    if (event.key.keysym.sym == keymap[i][0]) {
                        chip8->keys[keymap[i][1]] = 0;
                    }
                }
                break; 
        }
    }
    
    return quit;
}

int main(int argc, char** argv) {
    struct config config = {
    .delay_ms     = 2,
    .scale_factor = 15,
    .color_on     = 0x0f380fff,
    .color_off    = 0x9bbc0fff
    };

    args_parse(&config, argc, argv);
    
    const int window_width = CH8_VIDEO_WIDTH * config.scale_factor;
    const int window_height = CH8_VIDEO_HEIGHT * config.scale_factor;

    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow("ch8", 0, 0, window_width,
                                          window_height, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1,
                                                SDL_RENDERER_ACCELERATED);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                 SDL_TEXTUREACCESS_STREAMING, CH8_VIDEO_WIDTH, CH8_VIDEO_HEIGHT);
    
    bool quit = false;

    struct Chip8 chip8;
    chip8.color_on = config.color_on;
    chip8.color_off = config.color_off;
    
    chip8_initialize(&chip8);
    int ret = chip8_load_rom(&chip8, config.rom);
    if (ret < 0)
        exit(EXIT_FAILURE);

    while (!quit) { 
        usleep(1000 * config.delay_ms);

        quit = handle_input(&chip8);
        
        chip8_cycle(&chip8);
        
        SDL_UpdateTexture(texture, NULL, chip8.video, sizeof(chip8.video[0]) * CH8_VIDEO_WIDTH);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }
    
    return 0;
}
