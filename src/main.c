#include <stdio.h>
#include <stdbool.h>
#include <argp.h>
#include <SDL2/SDL.h>
#include <unistd.h>

#include "chip8.h"

/* ARGP STUFF */
const char* argp_program_version = "ch8 0.1";
const char* argp_program_bug_address = "<lambdcalcullus@gmail.com>";

struct arguments
{
    char*    rom;
    int      delay_ms;
    int      scale_factor;
    uint32_t color_on;
    uint32_t color_off;
};

struct argp_option options[] = {
    {"delay", 'd', "MILLIS", OPTION_ARG_OPTIONAL,
     "Delay for each cycle in milliseconds"},
    {"scale", 's', "SCALE", OPTION_ARG_OPTIONAL,
     "Integer scale factor for display"},
    {"color-on", 'C', "COLOR", OPTION_ARG_OPTIONAL,
    "Hex code for the color of on pixels (RGBA)"},
    {"color-off", 'c', "COLOR", OPTION_ARG_OPTIONAL,
    "Hex code for the color of off pixels (RGBA)"},
    {0}
};

static error_t
parse_opt(int key, char* arg, struct argp_state* state)
{
    struct arguments* arguments = state->input;

    switch (key) {
        /* TODO: deal with this
        case 'd':
            arguments->delay_ms = strtol(arg, (char**)NULL, 10);
            if (arguments->delay_ms == 0) {
                printf("a");
                argp_usage(state);
            }
            break;
        case 's':
            arguments->scale_factor = strtol(arg, (char**)NULL, 10);
            if (arguments->scale_factor <= 0) {
                printf("b");
                argp_usage(state);
            }
            break;
        case 'C':
            arguments->color_on = strtol(arg, (char**) NULL, 0);
            if (arguments->color_on <= 0) {
                printf("c");
                argp_usage(state);
            }
            break;
        case 'c': {
            arguments->color_off = strtol(arg, NULL, 0);
            if (arguments->color_off <= 0) {
                printf("d");
                argp_usage(state);
            }
            break;
        } */
        case ARGP_KEY_ARG:
            if (state->arg_num >= 1) {
                printf("Too many arguments.\n");
                argp_usage(state);
            }
            arguments->rom = arg;
            break;
        case ARGP_KEY_END:
            if (state->arg_num < 1) {
                printf("Not enough arguments.\n");
                argp_usage(state);
            }
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

static char args_doc[] = "[ROM]";

static char doc[] = 
"ch8 -- A CHIP-8 emulator written in C using SDL.";

static struct argp argp = {options, parse_opt, args_doc, doc};
/* END ARGP STUFF*/

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
    struct arguments arguments;

    arguments.delay_ms     = 2;
    arguments.scale_factor = 15;
    arguments.color_on     = 0x0f380fff;
    arguments.color_off    = 0x9bbc0fff;

    argp_parse(&argp, argc, argv, 0, 0, &arguments);
    
    const int window_width = CH8_VIDEO_WIDTH * arguments.scale_factor;
    const int window_height = CH8_VIDEO_HEIGHT * arguments.scale_factor;

    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow("ch8", 0, 0, window_width,
                                          window_height, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1,
                                                SDL_RENDERER_ACCELERATED);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                 SDL_TEXTUREACCESS_STREAMING, CH8_VIDEO_WIDTH, CH8_VIDEO_HEIGHT);
    
    bool quit = false;

    struct Chip8 chip8;
    chip8.color_on = arguments.color_on;
    chip8.color_off = arguments.color_off;
    
    chip8_initialize(&chip8);
    chip8_load_rom(&chip8, arguments.rom);

    while (!quit) { 
        usleep(1000 * arguments.delay_ms);

        quit = handle_input(&chip8);
        
        chip8_cycle(&chip8);
        
        SDL_UpdateTexture(texture, NULL, chip8.video, sizeof(chip8.video[0]) * CH8_VIDEO_WIDTH);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }
    
    return 0;
}
