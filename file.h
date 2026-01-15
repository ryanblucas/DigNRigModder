/*
	file.h ~ RL
	Interfaces with raw Dig-N-Rig game files
*/

#pragma once

#include "types.h"
/* remove this */
#include <Windows.h>

#define LAYER_COUNT 14
#define SAVE_BLOCK_STRUCT_SIZE 0x54
#define SAVE_MINERAL_STRUCT_SIZE 0x34
#define SAVE_MINERAL_MAX_COUNT 0xC350

/* This can be found at runtime by counting all the TileTypes that are == 0x7 */
#define DEFAULT_STALACTITE_COUNT 224

typedef enum mineral
{
	MINERAL_METALOID,
	MINERAL_ALUMON,
	MINERAL_EXPLODIUM,
	MINERAL_ENERGITE,
	MINERAL_RADIOACTON,
	MINERAL_ATOMICIDE,
	MINERAL_SCIPHIDE,
	MINERAL_RAREIEST,

	MINERAL_COUNT
} mineral_t;

/* Dig-N-Rig is a 32-bit application, meaning its pointers are 32-bits and NOT 64-bits */
typedef uint32_t dnr_pointer_t;
typedef int32_t boolean32_t;

typedef struct dnr_save_header
{
	int32_t total_block_count;
	int32_t current_block_count;
	int32_t mined_block_count;
	int32_t mined_mineral_count[MINERAL_COUNT];
} dnr_save_header_t;

typedef struct dnr_layer_header
{
	int32_t weather1;
	int32_t weather2;
	float weather3;
	int32_t total_block_count;
	int32_t current_block_count;
	uint32_t dirt_color;
} dnr_layer_header_t;

typedef struct dnr_sprite
{
	float x;
	float y;
	int32_t width;
	int32_t height;
	dnr_pointer_t ppchar_info_image;
	dnr_pointer_t ppint_tile;
	uint32_t dirt_color;
} dnr_sprite_t;

/* Reserved is stuff I haven't figured out yet */

typedef struct dnr_player
{
	dnr_sprite_t sprite;
	uint8_t reserved0[0x4C];
	int32_t fall_height;
	int32_t depth;
	float x_spawn;
	float y_spawn;
	uint8_t reserved1[0x18];
	int32_t health;
	int32_t max_health;
	uint8_t reserved2[0x2C];
	int32_t mineral_count[MINERAL_COUNT];
	int32_t fake_ladder_count;
	int32_t ladder_count;
	int32_t fake_platform_count;
	int32_t platform_count;
	int32_t fake_conveyor_count;
	int32_t conveyor_count;
	int32_t fake_scooper_count;
	int32_t scooper_count;
	int32_t item_hud_length;
	uint8_t reserved3[0x30];
} dnr_player_t;

typedef enum dnr_rig_type
{
	RIG_LADDER = 0x02,
	RIG_SCOOPER = 0x04,
	RIG_PLATFORM = 0x06,
	RIG_LAVA = 0x0A,
	RIG_WATER = 0x0B,
	RIG_CONVEYOR_LEFT = 0x0C,
	RIG_CONVEYOR_RIGHT = 0x0D
} dnr_rig_type_t;

typedef struct dnr_block
{
	uint8_t reserved0[0x8];
	int32_t layer_index;
	uint8_t reserved1[0xC];
	CHAR_INFO visual;
	uint8_t reserved2[0x4];
	boolean32_t block_exists;
	int32_t mineral_index;
	uint8_t reserved3[0x8];
	boolean32_t mineral_exists;
	dnr_rig_type_t rig_type;
	uint8_t reserved4[0x1C];
} dnr_block_t;

typedef struct dnr_mineral
{
	float x;
	float y;
	uint16_t size;
	uint16_t type;
	boolean32_t exists;
	uint8_t reserved0[0x24];
} dnr_mineral_t;

typedef struct stalactite
{
	float x;
	float y;
	float activation_radius_2;
	float speed;
	CHAR_INFO cell;
	boolean32_t falling;
	boolean32_t exists;
} stalactite_t;

#pragma pack(4)
typedef struct dnr_state
{
	dnr_save_header_t header;
	int32_t current_layer;
	int32_t diggit_version;
	int32_t times_won;
	int32_t seconds_played;
	dnr_layer_header_t layer_headers[LAYER_COUNT];
	dnr_player_t player;
	int32_t controls[21];
	dnr_block_t blocks[210000];
	dnr_mineral_t minerals[50000];
	uint8_t reserved0[0x30CAA0];
	boolean32_t elements_discovered[MINERAL_COUNT];
	uint8_t reserved1[0x1C];
	CHAR_INFO vac_pak_contents[1000];
	
	stalactite_t* stalactite_array;
	int stalactite_count;

	uint8_t reserved2[0x6AC];
	boolean32_t has_liquid_resistance;
} dnr_state_t;
#pragma pack()

sprite_t file_sprite_load(const char* directory);

dnr_state_t* file_state_load(const char* directory);
void file_state_unload(dnr_state_t* save);
void file_state_save(const char* directory, const dnr_state_t* save);

sprite_t file_state_spritify(const dnr_state_t* save, int layer_index);