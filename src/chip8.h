#ifndef CHIP8_H
#define CHIP8_H

#include <stdint.h>
#include <stdbool.h>

#define CH8_VIDEO_WIDTH  64
#define CH8_VIDEO_HEIGHT 32

struct Chip8 {
    uint16_t pc;
    uint16_t index;
    uint8_t  sp;
    uint16_t opcode;
    uint8_t  delay_timer;
    uint8_t  sound_timer;

    uint8_t  registers[16];
    uint8_t  memory[4096];
    uint16_t stack[16];
    uint32_t video[CH8_VIDEO_HEIGHT * CH8_VIDEO_WIDTH];
    bool     keys[16];
    
    // settings:
    /* TODO: should color be done outside? */
    uint32_t color_on;
    uint32_t color_off;
    /* TODO: add settings for quirks */
};

void chip8_initialize(struct Chip8* chip8);
int  chip8_load_rom(struct Chip8* chip8, char* filename);
void chip8_cycle(struct Chip8* chip8);
/* TODO: add setters for colors and keys */

#endif