#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "chip8.h"

#define FONTSET_SIZE    5 * 16
#define FONTSET_ADDRESS 0x50
#define START_ADDRESS   0x200
#define MAX_ROM_SIZE    4096 - START_ADDRESS

#define CLEAR(memory) memset(memory, 0, sizeof(memory))

#define FIRST_NIBBLE(opcode)  opcode >> 12u
#define SECOND_NIBBLE(opcode) (opcode >> 8u) & 0x0F
#define THIRD_NIBBLE(opcode)  (opcode >> 4u) & 0x0F
#define FOURTH_NIBBLE(opcode) opcode & 0x0F

static uint8_t chip8_fontset[FONTSET_SIZE] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x20, 0x60, 0x20, 0x20, 0x70, // 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

//function pointer tables
void (*main_table[0x10])(struct Chip8*);
void (*table0[0x10])(struct Chip8*);
void (*table8[0x10])(struct Chip8*);
void (*tableE[0x10])(struct Chip8*);
void (*tableF[0x100])(struct Chip8*);


/* BEGIN OPCODES */
static void opcode_NULL(struct Chip8* chip8)
{
	//do nothing
}

//CLR
static void opcode_00E0(struct Chip8* chip8)
{
	for (int i = 0; i < CH8_VIDEO_WIDTH; i++) {
		for (int j = 0; j < CH8_VIDEO_HEIGHT; j++) {
			chip8->video[j * CH8_VIDEO_WIDTH + i] = chip8->color_off;
		}
	}
}

//RET
static void opcode_00EE(struct Chip8* chip8)
{
	chip8->pc = chip8->stack[--chip8->sp];
}

//JMPI addr
static void opcode_1NNN(struct Chip8* chip8)
{
	chip8->pc = chip8->opcode & 0x0FFF;
}

//CALL addr
static void opcode_2NNN(struct Chip8* chip8)
{
	chip8->stack[chip8->sp++] = chip8->pc;
	chip8->pc = chip8->opcode & 0x0FFF;
}

//SEI Vx, imm
static void opcode_3XNN(struct Chip8* chip8)
{
	uint8_t x   = SECOND_NIBBLE(chip8->opcode);
	uint8_t imm = chip8->opcode & 0x00FF;
	if (chip8->registers[x] == imm)
		chip8->pc += 2;
}

//SNEI Vx, imm
static void opcode_4XNN(struct Chip8* chip8)
{
	uint8_t x   = SECOND_NIBBLE(chip8->opcode);
	uint8_t imm = chip8->opcode & 0x00FF;
	if (chip8->registers[x] != imm)
		chip8->pc += 2;
}

//SE Vx, Vy
static void opcode_5XY0(struct Chip8* chip8)
{
	uint8_t x = SECOND_NIBBLE(chip8->opcode);
	uint8_t y = THIRD_NIBBLE(chip8->opcode);
	if (chip8->registers[x] == chip8->registers[y])
	{
		chip8->pc += 2;
	}
}

//LDI Vx, imm
static void opcode_6XNN(struct Chip8* chip8)
{
	uint8_t x = SECOND_NIBBLE(chip8->opcode);
	uint8_t imm = chip8->opcode & 0x00FF;
	chip8->registers[x] = imm;
}

//ADDI Vx, imm
static void opcode_7XNN(struct Chip8* chip8)
{
	uint8_t x = SECOND_NIBBLE(chip8->opcode);
	uint8_t imm = chip8->opcode & 0x00FF;
	chip8->registers[x] += imm;
}

//MOV Vx, Vy
static void opcode_8XY0(struct Chip8* chip8)
{
	uint8_t x = SECOND_NIBBLE(chip8->opcode);
	uint8_t y = THIRD_NIBBLE(chip8->opcode);
	chip8->registers[x] = chip8->registers[y];

}

//OR Vx, Vy
static void opcode_8XY1(struct Chip8* chip8)
{
	uint8_t x = SECOND_NIBBLE(chip8->opcode);
	uint8_t y = THIRD_NIBBLE(chip8->opcode);
	chip8->registers[x] |= chip8->registers[y];

}

//AND Vx, Vy
static void opcode_8XY2(struct Chip8* chip8)
{
	uint8_t x = SECOND_NIBBLE(chip8->opcode);
	uint8_t y = THIRD_NIBBLE(chip8->opcode);
	chip8->registers[x] &= chip8->registers[y];

}

//XOR Vx, Vy
static void opcode_8XY3(struct Chip8* chip8)
{
	uint8_t x = SECOND_NIBBLE(chip8->opcode);
	uint8_t y = THIRD_NIBBLE(chip8->opcode);
	chip8->registers[x] ^= chip8->registers[y];

}

//ADD Vx, Vy
static void opcode_8XY4(struct Chip8* chip8)
{
	uint8_t x = SECOND_NIBBLE(chip8->opcode);
	uint8_t y = THIRD_NIBBLE(chip8->opcode);
	uint16_t sum = chip8->registers[x] + chip8->registers[y];

	if (sum > 0xFF)
		chip8->registers[0xF] = 1;
	else
	 	chip8->registers[0xF] = 0;

	chip8->registers[x] = sum & 0xFF;
}

//SUB Vx, Vy
static void opcode_8XY5(struct Chip8* chip8)
{
	uint8_t x = SECOND_NIBBLE(chip8->opcode);
	uint8_t y = THIRD_NIBBLE(chip8->opcode);
	
	if (chip8->registers[x] > chip8->registers[y])
		chip8->registers[0xF] = 1;
	else
		chip8->registers[0xF] = 0;
	
	chip8->registers[x] -= chip8->registers[y];

}

//SHR Vx, Vy?
static void opcode_8XY6(struct Chip8* chip8)
{
	/* TODO: implementations of this differ. make configurable? */
	uint8_t x = SECOND_NIBBLE(chip8->opcode);
	
	chip8->registers[0xF] = chip8->registers[x] & 0x1;
	
	chip8->registers[x] >>= 1;	
}

//NSUB Vx, Vy
static void opcode_8XY7(struct Chip8* chip8)
{
	uint8_t x = SECOND_NIBBLE(chip8->opcode);
	uint8_t y = THIRD_NIBBLE(chip8->opcode);
	
	if (chip8->registers[x] < chip8->registers[y])
		chip8->registers[0xF] = 1;
	else
		chip8->registers[0xF] = 0;
	
	chip8->registers[x] = chip8->registers[y] - chip8->registers[x];
}

//SHL Vx, Vy?
static void opcode_8XYE(struct Chip8* chip8)
{
	/* TODO: implementations of this differ. make configurable? */
	uint8_t x = SECOND_NIBBLE(chip8->opcode);
	
	chip8->registers[0xF] = chip8->registers[x] & 0b10000000;
	
	chip8->registers[x] <<= 1;	
}

//SNE Vx, Vy
static void opcode_9XY0(struct Chip8* chip8)
{
	if (chip8->registers[SECOND_NIBBLE(chip8->opcode)] !=
	    chip8->registers[THIRD_NIBBLE(chip8->opcode)]) 
	{
		chip8->pc += 2;
	}
	
}

//LD I, Vx
static void opcode_ANNN(struct Chip8* chip8)
{
	chip8->index = chip8->opcode & 0x0FFF;
}

//JMP V0, addr
static void opcode_BNNN(struct Chip8* chip8)
{
	/* TODO: implementations of this differ. make configurable? */
	chip8->pc = chip8->opcode & 0x0FFF + chip8->registers[0];
}

//RND Vx, imm
static void opcode_CXNN(struct Chip8* chip8)
{
	uint8_t x      = SECOND_NIBBLE(chip8->opcode);
	uint8_t imm    = chip8->opcode & 0x00FF; 
	uint8_t random = rand() & 0xFF;
	
	chip8->registers[x] = random & imm;
}

//DRW Vx, Vy, imm
static void opcode_DXYN(struct Chip8* chip8)
{
	uint8_t x   = SECOND_NIBBLE(chip8->opcode);
	uint8_t y   = THIRD_NIBBLE(chip8->opcode);
	uint8_t imm = FOURTH_NIBBLE(chip8->opcode);

	uint8_t x_pos = chip8->registers[x] % CH8_VIDEO_WIDTH;
	uint8_t y_pos = chip8->registers[y] % CH8_VIDEO_HEIGHT;
	
	chip8->registers[0xF] = 0;
	for (int i = 0; i < imm; i++) {
		uint8_t sprite_byte = chip8->memory[chip8->index + i];

		for (int j = 0; j < 8; j++) {
			uint16_t pixel_address = (y_pos + i) * CH8_VIDEO_WIDTH + (x_pos + j); 
			
			bool sprite_pixel = (sprite_byte >> (7-j)) & 0x1; 
			bool video_pixel  = (chip8->video[pixel_address] == chip8->color_on);
			
			if ((sprite_pixel == 0x1) & (video_pixel == 0x1))
				chip8->registers[0xF] = 0x1;

			chip8->video[pixel_address] = (video_pixel ^ sprite_pixel) 
										? chip8->color_on
										: chip8->color_off;			

			if ((x_pos + j) == (CH8_VIDEO_WIDTH - 1))
				break;
		}
		if ((y_pos + i) == (CH8_VIDEO_HEIGHT - 1))
			break;
	}
}

//SKP Vx
static void opcode_EX9E (struct Chip8* chip8)
{
	uint8_t x = SECOND_NIBBLE(chip8->opcode);
	//could cause an issue if value in Vx is too big
	//but other people don't care, so i guess i don't either
	if (chip8->keys[chip8->registers[x]])
		chip8->pc += 2;
	
}

//SKNP Vx
static void opcode_EXA1 (struct Chip8* chip8) {
	uint8_t x = SECOND_NIBBLE(chip8->opcode);
	//could cause an issue if value in Vx is too big
	if (!chip8->keys[chip8->registers[x]])
		chip8->pc += 2;	
}

//LD Vx, DT
static void opcode_FX07(struct Chip8* chip8)
{
	uint8_t x = SECOND_NIBBLE(chip8->opcode);
	chip8->registers[x] = chip8->delay_timer;
}

//LD DT, Vx
static void opcode_FX15(struct Chip8* chip8)
{
	uint8_t x = SECOND_NIBBLE(chip8->opcode);
	chip8->delay_timer = chip8->registers[x];
}

//LD ST, Vx
static void opcode_FX18(struct Chip8* chip8)
{
	uint8_t x = SECOND_NIBBLE(chip8->opcode);
	chip8->sound_timer = chip8->registers[x];
}

//ADD I, Vx
static void opcode_FX1E(struct Chip8* chip8)
{
	/* TODO: aaaaaa this might also depend on implementation WHYYYY*/
	uint8_t x = SECOND_NIBBLE(chip8->opcode);
	uint16_t sum = chip8->index + chip8->registers[x];

	if (sum > 0x100)
		chip8->registers[0xF] = 1;
	chip8->index = sum & 0x0FFF;
}

//KEY Vx
static void opcode_FX0A(struct Chip8* chip8)
{
	uint8_t x = SECOND_NIBBLE(chip8->opcode);
	int block = 1;
	for (int i = 0; i < 16; i++) {
		if (chip8->keys[i] == 1) {
			block = 0;
			chip8->registers[x] = chip8->keys[i];
			break;
		}

	}
	
	if (block)
		chip8->pc -= 2;
}

//LDF Vx
static void opcode_FX29(struct Chip8* chip8)
{
	uint8_t x = SECOND_NIBBLE(chip8->opcode);
	uint8_t font_char = chip8->registers[x] & 0x0F;
	
	chip8->index = FONTSET_ADDRESS + font_char;
}

//BTD Vx
static void opcode_FX33(struct Chip8* chip8)
{
	uint8_t x = SECOND_NIBBLE(chip8->opcode);
	uint8_t decimal = chip8->registers[x];
	uint8_t address = chip8->index;

	chip8->memory[address + 2] = decimal % 10;
	decimal /= 10;
	chip8->memory[address + 1] = decimal % 10;
	decimal /= 10;
	chip8->memory[address] = decimal;
}

//ST [I], Vx
static void opcode_FX55(struct Chip8* chip8)
{
	uint8_t x = SECOND_NIBBLE(chip8->opcode);
	uint8_t address = chip8->index;

	for (int i = 0; i <= x; i++) {
		chip8->memory[address + i] = chip8->registers[i];
	}
}

//LD Vx, [I]
static void opcode_FX65(struct Chip8* chip8)
{
	uint8_t x = SECOND_NIBBLE(chip8->opcode);
	uint8_t address = chip8->index;

	for (int i = 0; i <= x; i++) {
		chip8->registers[i] = chip8->memory[address + i];
	}
}
/* END OPCODES */

/* TABLE STUFF */
static void decode_table0(struct Chip8* chip8)
{
	table0[FOURTH_NIBBLE(chip8->opcode)](chip8);
}

static void decode_table8(struct Chip8* chip8)
{
	table8[FOURTH_NIBBLE(chip8->opcode)](chip8);	
}

static void decode_tableE(struct Chip8* chip8)
{
	tableE[FOURTH_NIBBLE(chip8->opcode)](chip8);
}

static void decode_tableF(struct Chip8* chip8)
{
	tableF[chip8->opcode & 0xFF](chip8);
}

static void (*decode_opcode(uint16_t opcode))(struct Chip8*)
{
	return main_table[FIRST_NIBBLE(opcode)];
}

static void init_tables(void)
{
	main_table[0x0] = decode_table0;
	main_table[0x1] = opcode_1NNN;
	main_table[0x2] = opcode_2NNN;
	main_table[0x3] = opcode_3XNN;
	main_table[0x4] = opcode_4XNN;
	main_table[0x5] = opcode_5XY0;
	main_table[0x6] = opcode_6XNN;
	main_table[0x7] = opcode_7XNN;
	main_table[0x8] = decode_table8;
	main_table[0x9] = opcode_9XY0;
	main_table[0xA] = opcode_ANNN;
	main_table[0xB] = opcode_BNNN;
	main_table[0xC] = opcode_CXNN;
	main_table[0xD] = opcode_DXYN;
	main_table[0xE] = decode_tableE;
	main_table[0xF] = decode_tableF;

	for (int i = 0; i < 0x10; i++) {
		table0[i] = opcode_NULL;
		table8[i] = opcode_NULL;
		tableE[i] = opcode_NULL;
	}
	
	table0[0x0] = opcode_00E0;
	table0[0xE] = opcode_00EE;

	table8[0x0] = opcode_8XY0;
	table8[0x1] = opcode_8XY1;
	table8[0x2] = opcode_8XY2;
	table8[0x3] = opcode_8XY3;
	table8[0x4] = opcode_8XY4;
	table8[0x5] = opcode_8XY5;
	table8[0x6] = opcode_8XY6;
	table8[0x7] = opcode_8XY7;
	table8[0xE] = opcode_8XYE;
	
	tableE[0x1] = opcode_EXA1;
	tableE[0xE] = opcode_EX9E;
	
	for (int i = 0; i < 0x100; i++) {
		tableF[i] = opcode_NULL;
	}

	tableF[0x07] = opcode_FX07;
	tableF[0x0A] = opcode_FX0A;
	tableF[0x15] = opcode_FX15;
	tableF[0x18] = opcode_FX18;
	tableF[0x1E] = opcode_FX1E;
	tableF[0x29] = opcode_FX29;
	tableF[0x33] = opcode_FX33;
	tableF[0x55] = opcode_FX55;
	tableF[0x65] = opcode_FX65;
}
/* END OF TABLE STUFF */


/* EXPORTED FUNCS */
void chip8_initialize(struct Chip8* chip8)
{
	chip8->pc          = START_ADDRESS;
	chip8->index       = 0;
	chip8->sp          = 0;
	chip8->opcode      = 0;	
	chip8->delay_timer = 0;
	chip8->sound_timer = 0;

	CLEAR(chip8->registers);
	CLEAR(chip8->memory);
	CLEAR(chip8->stack);
	CLEAR(chip8->keys);
	
	for (int i = 0; i < CH8_VIDEO_WIDTH; i++) {
		for (int j = 0; j < CH8_VIDEO_HEIGHT; j++) {
			chip8->video[j * CH8_VIDEO_WIDTH + i] = chip8->color_off;
		}
	}
	
	init_tables();

	srand(time(NULL));

	for (int i = 0; i < FONTSET_SIZE; ++i) {
		chip8->memory[FONTSET_ADDRESS + i] = chip8_fontset[i];
	}
}

int chip8_load_rom(struct Chip8* chip8, char* filename) {	
	FILE* fp = fopen(filename, "rb");
	if (!fp) {
		printf("Error opening ROM.\n");
		fclose(fp);
		return -1;
	}

	fseek(fp, 0, SEEK_END);
	long file_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if (file_size > MAX_ROM_SIZE) {
		printf("Error reading ROM: File too big. (maximum size is %dB)\n",
				MAX_ROM_SIZE);
		fclose(fp);
		return -1;
	}
	
	size_t ret_code = fread(&chip8->memory[START_ADDRESS], 1, MAX_ROM_SIZE, fp);
	if (!ret_code) {
		printf("Error reading file.\n");
		fclose(fp);
		return -1;
	}

	fclose(fp);
	return 0;
}

void chip8_cycle(struct Chip8* chip8) {
	chip8->opcode = (chip8->memory[chip8->pc] << 8u) |
	                (chip8->memory[chip8->pc+1]);
	chip8->pc += 2;
	
	(*decode_opcode(chip8->opcode))(chip8);

	if (chip8->delay_timer) 
		chip8->delay_timer--;
	if (chip8->sound_timer)
		chip8->sound_timer--;
}