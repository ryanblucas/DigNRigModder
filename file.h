/*
	file.h ~ RL
	Interfaces with raw Dig-N-Rig game files
*/

#pragma once

#include "types.h"
/* remove this */
#include <Windows.h>

#define SAVE_BLOCK_STRUCT_SIZE 0x54
#define SAVE_MINERAL_STRUCT_SIZE 0x34
#define SAVE_MINERAL_MAX_COUNT 0xC350

/* This can be found at runtime by counting all the TileTypes that are == 0x7 */
#define DEFAULT_STALACTITE_COUNT 242

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

#define ADD_SERIALIZABLE(type, name) type name;
#define ADD_SERIALIZABLE_ARRAY(type, name, count) type name[count];
#define ADD_SERIALIZABLE_ENUM(name, value) name = value,

typedef struct stalactite
{
#define SERIALIZABLE_STALACTITE \
	ADD_SERIALIZABLE(float, x) \
	ADD_SERIALIZABLE(float, y) \
	ADD_SERIALIZABLE(float, activation_radius_2) \
	ADD_SERIALIZABLE(float, speed) \
	ADD_SERIALIZABLE(boolean32_t, falling) \
	ADD_SERIALIZABLE(boolean32_t, exists) \
	ADD_SERIALIZABLE(CHAR_INFO, cell)
SERIALIZABLE_STALACTITE
} stalactite_t;

#pragma pack(4)

typedef struct dnr_save_header
{
#define SERIALIZABLE_DNR_SAVE_HEADER \
	ADD_SERIALIZABLE(int32_t, total_block_count) \
	ADD_SERIALIZABLE(int32_t, current_block_count) \
	ADD_SERIALIZABLE(int32_t, mined_block_count) \
	ADD_SERIALIZABLE_ARRAY(int32_t, mined_mineral_count, MINERAL_COUNT)
SERIALIZABLE_DNR_SAVE_HEADER
} dnr_save_header_t;

typedef struct dnr_layer_header
{
#define SERIALIZABLE_DNR_LAYER_HEADER \
	ADD_SERIALIZABLE(int32_t, weather1) \
	ADD_SERIALIZABLE(int32_t, weather2) \
	ADD_SERIALIZABLE(float, weather3) \
	ADD_SERIALIZABLE(int32_t, total_block_count) \
	ADD_SERIALIZABLE(int32_t, current_block_count) \
	ADD_SERIALIZABLE(rgb_color_t, dirt_color)
SERIALIZABLE_DNR_LAYER_HEADER
} dnr_layer_header_t;

typedef struct dnr_sprite
{
#define SERIALIZABLE_DNR_SPRITE \
	ADD_SERIALIZABLE(float, x) \
	ADD_SERIALIZABLE(float, y) \
	ADD_SERIALIZABLE(int32_t, width) \
	ADD_SERIALIZABLE(int32_t, height) \
	ADD_SERIALIZABLE(dnr_pointer_t, ppchar_info_image) \
	ADD_SERIALIZABLE(dnr_pointer_t, ppint_tile) \
	ADD_SERIALIZABLE(rgb_color_t, dirt_color)
SERIALIZABLE_DNR_SPRITE
} dnr_sprite_t;

/* Reserved is stuff I haven't figured out yet */

typedef struct dnr_player
{
#define SERIALIZABLE_DNR_PLAYER \
	ADD_SERIALIZABLE(dnr_sprite_t, sprite) \
	ADD_SERIALIZABLE_ARRAY(uint8_t, reserved0, 0x4C) \
	ADD_SERIALIZABLE(int32_t, fall_height) \
	ADD_SERIALIZABLE(int32_t, depth) \
	ADD_SERIALIZABLE(float, x_spawn) \
	ADD_SERIALIZABLE(float, y_spawn) \
	ADD_SERIALIZABLE_ARRAY(uint8_t, reserved1, 0x18) \
	ADD_SERIALIZABLE(int32_t, health) \
	ADD_SERIALIZABLE(int32_t, max_health) \
	ADD_SERIALIZABLE_ARRAY(uint8_t, reserved2, 0x2C) \
	ADD_SERIALIZABLE_ARRAY(int32_t, mineral_count, MINERAL_COUNT) \
	ADD_SERIALIZABLE(int32_t, fake_ladder_count) \
	ADD_SERIALIZABLE(int32_t, ladder_count) \
	ADD_SERIALIZABLE(int32_t, fake_platform_count) \
	ADD_SERIALIZABLE(int32_t, platform_count) \
	ADD_SERIALIZABLE(int32_t, fake_conveyor_count) \
	ADD_SERIALIZABLE(int32_t, conveyor_count) \
	ADD_SERIALIZABLE(int32_t, fake_scooper_count) \
	ADD_SERIALIZABLE(int32_t, scooper_count) \
	ADD_SERIALIZABLE(int32_t, item_hud_length) \
	ADD_SERIALIZABLE_ARRAY(uint8_t, reserved3, 0x30)
SERIALIZABLE_DNR_PLAYER
} dnr_player_t;

typedef enum dnr_rig_type
{
#define SERIALIZABLE_DNR_RIG_TYPE \
	ADD_SERIALIZABLE_ENUM(RIG_NONE, 0x00) \
	ADD_SERIALIZABLE_ENUM(RIG_LADDER_CENTER, 0x01) \
	ADD_SERIALIZABLE_ENUM(RIG_LADDER, 0x02) \
	ADD_SERIALIZABLE_ENUM(RIG_SCOOPER, 0x04) \
	ADD_SERIALIZABLE_ENUM(RIG_PLATFORM, 0x06) \
	ADD_SERIALIZABLE_ENUM(RIG_DIRT, 0x07) \
	ADD_SERIALIZABLE_ENUM(RIG_STONE, 0x08) \
	ADD_SERIALIZABLE_ENUM(RIG_BRICK, 0x09) \
	ADD_SERIALIZABLE_ENUM(RIG_LAVA, 0x0A) \
	ADD_SERIALIZABLE_ENUM(RIG_WATER, 0x0B) \
	ADD_SERIALIZABLE_ENUM(RIG_CONVEYOR_LEFT, 0x0C) \
	ADD_SERIALIZABLE_ENUM(RIG_CONVEYOR_RIGHT, 0x0D) \
	ADD_SERIALIZABLE_ENUM(RIG_BACKGROUND, 0x0F)
SERIALIZABLE_DNR_RIG_TYPE
} dnr_rig_type_t;

typedef enum dnr_mineral_move_direction
{
#define SERIALIZABLE_DNR_MINERAL_MOVE_DIRECTION \
	ADD_SERIALIZABLE_ENUM(MOVE_DIRECTION_LEFT, 0x00) \
	ADD_SERIALIZABLE_ENUM(MOVE_DIRECTION_RIGHT, 0x01) \
	ADD_SERIALIZABLE_ENUM(MOVE_DIRECTION_UP, 0x02) \
	ADD_SERIALIZABLE_ENUM(MOVE_DIRECTION_DOWN, 0x04)
	SERIALIZABLE_DNR_MINERAL_MOVE_DIRECTION
} dnr_mineral_move_direction_t;

typedef enum dnr_mineral_spawn_rule
{
#define SERIALIZABLE_DNR_MINERAL_SPAWN_RULE \
	ADD_SERIALIZABLE_ENUM(MINERAL_SPAWN_METALOID, 0x00) \
	ADD_SERIALIZABLE_ENUM(MINERAL_SPAWN_EXPLODIUM, 0x01) \
	ADD_SERIALIZABLE_ENUM(MINERAL_SPAWN_ALUMON, 0x02) \
	ADD_SERIALIZABLE_ENUM(MINERAL_SPAWN_SCIPHIDE, 0x03) \
	ADD_SERIALIZABLE_ENUM(MINERAL_SPAWN_ENERGITE, 0x04) \
	ADD_SERIALIZABLE_ENUM(MINERAL_SPAWN_RADIOACTON, 0x05) \
	ADD_SERIALIZABLE_ENUM(MINERAL_SPAWN_ATOMICIDE, 0x06) \
	ADD_SERIALIZABLE_ENUM(MINERAL_SPAWN_RAREIEST, 0x07) \
	ADD_SERIALIZABLE_ENUM(MINERAL_SPAWN_NOTHING, 0x08) \
	ADD_SERIALIZABLE_ENUM(MINERAL_SPAWN_RANDOM, 0x09)
	SERIALIZABLE_DNR_MINERAL_SPAWN_RULE
} dnr_mineral_spawn_rule_t;

typedef struct dnr_block
{
#define SERIALIZABLE_DNR_BLOCK \
	ADD_SERIALIZABLE(int32_t, x) \
	ADD_SERIALIZABLE(int32_t, y) \
	ADD_SERIALIZABLE(int32_t, layer_index) \
	ADD_SERIALIZABLE(int32_t, health_percentage) \
	ADD_SERIALIZABLE(int32_t, health_max) \
	ADD_SERIALIZABLE(int32_t, health_current) \
	ADD_SERIALIZABLE(CHAR_INFO, visual) \
	ADD_SERIALIZABLE(CHAR_INFO, secondary_visual) \
	ADD_SERIALIZABLE(boolean32_t, block_exists) \
	ADD_SERIALIZABLE(int32_t, mineral_index) \
	ADD_SERIALIZABLE(dnr_pointer_t, enemy_pointer) \
	ADD_SERIALIZABLE(boolean32_t, can_mine) \
	ADD_SERIALIZABLE(boolean32_t, mineral_exists) \
	ADD_SERIALIZABLE(dnr_rig_type_t, rig_type) \
	ADD_SERIALIZABLE(boolean32_t, enemy_exists) \
	ADD_SERIALIZABLE(dnr_mineral_move_direction_t, mineral_move_direction) \
	ADD_SERIALIZABLE(boolean32_t, should_animate_conveyor) \
	ADD_SERIALIZABLE(int32_t, damage) \
	ADD_SERIALIZABLE(dnr_mineral_spawn_rule_t, mineral_spawn_rule) \
	ADD_SERIALIZABLE(boolean32_t, can_remove_rig) \
	ADD_SERIALIZABLE(uint32_t, mineable_attribute) 
SERIALIZABLE_DNR_BLOCK
} dnr_block_t;

enum dnr_mineral_size
{
#define SERIALIZABLE_DNR_MINERAL_SIZE \
	ADD_SERIALIZABLE_ENUM(MINERAL_SIZE_5, 0xE0) \
	ADD_SERIALIZABLE_ENUM(MINERAL_SIZE_4, 0xE1) \
	ADD_SERIALIZABLE_ENUM(MINERAL_SIZE_3, 0xE2) \
	ADD_SERIALIZABLE_ENUM(MINERAL_SIZE_2, 0xE3) \
	ADD_SERIALIZABLE_ENUM(MINERAL_SIZE_1, 0xE4) \
	ADD_SERIALIZABLE_ENUM(MINERAL_SIZE_0, 0xE5)
SERIALIZABLE_DNR_MINERAL_SIZE
};
typedef uint8_t dnr_mineral_size_t;

enum dnr_mineral_type
{
#define SERIALIZABLE_DNR_MINERAL_TYPE \
	ADD_SERIALIZABLE_ENUM(MINERAL_TYPE_METALOID, 0x01) \
	ADD_SERIALIZABLE_ENUM(MINERAL_TYPE_ALUMON, 0x0B) \
	ADD_SERIALIZABLE_ENUM(MINERAL_TYPE_EXPLODIUM, 0x0C) \
	ADD_SERIALIZABLE_ENUM(MINERAL_TYPE_ENERGITE, 0x0E) \
	ADD_SERIALIZABLE_ENUM(MINERAL_TYPE_RADIOACTON, 0x0A) \
	ADD_SERIALIZABLE_ENUM(MINERAL_TYPE_ATOMICIDE, 0x09) \
	ADD_SERIALIZABLE_ENUM(MINERAL_TYPE_SCIPHIDE, 0x0D) \
	ADD_SERIALIZABLE_ENUM(MINERAL_TYPE_RAREIEST, 0x0F)
SERIALIZABLE_DNR_MINERAL_TYPE
};
typedef uint16_t dnr_mineral_type_t;

typedef struct dnr_mineral
{
#define SERIALIZABLE_DNR_MINERAL \
	ADD_SERIALIZABLE(float, x) \
	ADD_SERIALIZABLE(float, y) \
	ADD_SERIALIZABLE(dnr_mineral_size_t, size) \
	/* PADDING IMPLICIT BY COMPILER */ \
	ADD_SERIALIZABLE(dnr_mineral_type_t, type) \
	ADD_SERIALIZABLE(boolean32_t, exists) \
	ADD_SERIALIZABLE(int32_t, lifetime) \
	ADD_SERIALIZABLE(uint32_t, ticks_trying_to_move) \
	ADD_SERIALIZABLE(uint32_t, index) \
	ADD_SERIALIZABLE(uint32_t, undefined4_00) \
	ADD_SERIALIZABLE(float, x_velocity) \
	ADD_SERIALIZABLE(float, y_velocity) \
	ADD_SERIALIZABLE(float, prev_x_velocity) \
	ADD_SERIALIZABLE(float, prev_y_velocity) \
	ADD_SERIALIZABLE(int32_t, int_01)
SERIALIZABLE_DNR_MINERAL
} dnr_mineral_t;

#pragma pack(1)

typedef struct dnr_state
{
#define SERIALIZABLE_DNR_STATE_0 \
	ADD_SERIALIZABLE(dnr_save_header_t, header) \
	ADD_SERIALIZABLE(int32_t, current_layer) \
	ADD_SERIALIZABLE(int32_t, diggit_version) \
	ADD_SERIALIZABLE(int32_t, times_won) \
	ADD_SERIALIZABLE(int32_t, seconds_played) \
	ADD_SERIALIZABLE_ARRAY(dnr_layer_header_t, layer_headers, LAYER_COUNT) \
	ADD_SERIALIZABLE(dnr_player_t, player) \
	ADD_SERIALIZABLE_ARRAY(int32_t, controls, 21)
SERIALIZABLE_DNR_STATE_0

	/* too large and unmanageable to put in the tree view */

	dnr_block_t blocks[210000];
	dnr_mineral_t minerals[50000];
	uint8_t enemies[0x30CAA0];

#define SERIALIZABLE_DNR_STATE_1 \
	ADD_SERIALIZABLE_ARRAY(boolean32_t, elements_discovered, MINERAL_COUNT) \
	ADD_SERIALIZABLE_ARRAY(uint8_t, reserved1, 0x1C) \
	ADD_SERIALIZABLE_ARRAY(CHAR_INFO, vac_pak_contents, 1000)
SERIALIZABLE_DNR_STATE_1

	stalactite_t* stalactite_array;
	int stalactite_count;

#define SERIALIZABLE_DNR_STATE_2 \
	ADD_SERIALIZABLE_ARRAY(uint8_t, reserved2, 0x6AC) \
	ADD_SERIALIZABLE(boolean32_t, has_liquid_resistance)
SERIALIZABLE_DNR_STATE_2
} dnr_state_t;

#pragma pack()

#undef ADD_SERIALIZABLE
#undef ADD_SERIALIZABLE_ARRAY
#undef ADD_SERIALIZABLE_ENUM

sprite_t file_sprite_load(const char* directory);

dnr_state_t* file_state_load(const char* directory);
void file_state_unload(dnr_state_t* save);
bool file_state_save(const char* directory, const dnr_state_t* save);

CHAR_INFO file_state_spritify_cell(const dnr_state_t* save, int x, int y);
sprite_t file_state_spritify(const dnr_state_t* save, int layer_index);