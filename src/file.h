/*
	file.h ~ RL
	Interfaces with raw Dig-N-Rig game files
*/

#pragma once

#include "types.h"
#include <Windows.h>

/* This can be found at runtime by counting all the TileTypes that are == 0x7 */
#define DEFAULT_STALACTITE_COUNT 242
#define MOD_FOOTER_SIZE 267

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

typedef struct shop_item
{
#define SERIALIZABLE_SHOP_ITEM \
	ADD_SERIALIZABLE(boolean32_t, discovered) \
	ADD_SERIALIZABLE(int32_t, discovery_percentage) \
	ADD_SERIALIZABLE(int32_t, count_max) \
	ADD_SERIALIZABLE(int32_t, count_next) \
	ADD_SERIALIZABLE(int32_t, count_curr) \
	ADD_SERIALIZABLE_ARRAY(int32_t, mineral_cost, MINERAL_COUNT)
SERIALIZABLE_SHOP_ITEM
} shop_item_t;

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

typedef enum dnr_weather_type
{
#define SERIALIZABLE_DNR_WEATHER_TYPE \
	ADD_SERIALIZABLE_ENUM(WEATHER_NONE, 0x00) \
	ADD_SERIALIZABLE_ENUM(WEATHER_RAIN, 0x01) \
	ADD_SERIALIZABLE_ENUM(WEATHER_LAVA, 0x02) \
	ADD_SERIALIZABLE_ENUM(WEATHER_RAIN_AND_LAVA, 0x03)
SERIALIZABLE_DNR_WEATHER_TYPE
} dnr_weather_type_t;

typedef struct dnr_layer_header
{
#define SERIALIZABLE_DNR_LAYER_HEADER \
	ADD_SERIALIZABLE(dnr_weather_type_t, weather_type) \
	ADD_SERIALIZABLE(int32_t, weather_particle_rate) \
	ADD_SERIALIZABLE(float, weather_speed) \
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
	ADD_SERIALIZABLE_ARRAY(uint32_t, reserved1, 0x2) \
	ADD_SERIALIZABLE(float, jetpack_capacity) \
	ADD_SERIALIZABLE_ARRAY(uint32_t, reserved2, 0x2) \
	ADD_SERIALIZABLE(int32_t, hud_battery_count) \
	ADD_SERIALIZABLE(int32_t, health) \
	ADD_SERIALIZABLE(int32_t, max_health) \
	ADD_SERIALIZABLE_ARRAY(uint8_t, reserved3, 0x2C) \
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
	ADD_SERIALIZABLE(int32_t, battery_count) \
	ADD_SERIALIZABLE(int32_t, undefined4_00) \
	ADD_SERIALIZABLE(int32_t, dynamite_count) \
	ADD_SERIALIZABLE(int32_t, double_dynamite_count) \
	ADD_SERIALIZABLE(int32_t, mega_bomb_count) \
	ADD_SERIALIZABLE(int32_t, dirtzooka_count) \
	ADD_SERIALIZABLE_ARRAY(uint8_t, reserved4, 0x18)
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

#pragma pack(4)

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
	ADD_SERIALIZABLE_ARRAY(boolean32_t, unused_elements_discovered, MINERAL_COUNT) \
	ADD_SERIALIZABLE(int32_t, vac_pak_size) \
	ADD_SERIALIZABLE(int32_t, vac_pak_capacity) \
	ADD_SERIALIZABLE(boolean32_t, vac_pak_exists) \
	ADD_SERIALIZABLE(int16_t, vac_pak_x) \
	ADD_SERIALIZABLE(boolean32_t, vac_pak_not_moving) \
	ADD_SERIALIZABLE(int32_t, vac_pak_range_squared) \
	ADD_SERIALIZABLE(boolean32_t, can_use_vacuum) \
	ADD_SERIALIZABLE_ARRAY(CHAR_INFO, vac_pak_contents, 1000)
SERIALIZABLE_DNR_STATE_1

	stalactite_t* stalactite_array;
	int stalactite_count;

#define SERIALIZABLE_DNR_STATE_2 \
	ADD_SERIALIZABLE(uint32_t, style_content_attrib) \
	ADD_SERIALIZABLE(uint32_t, style_header_attrib) \
	ADD_SERIALIZABLE(uint32_t, style_horizontal_char) \
	ADD_SERIALIZABLE(uint32_t, style_vertical_char) \
	ADD_SERIALIZABLE(uint32_t, style_joint_char) \
	ADD_SERIALIZABLE_ARRAY(boolean32_t, elements_discovered, MINERAL_COUNT) \
	ADD_SERIALIZABLE(int32_t, count_elements_discovered)
SERIALIZABLE_DNR_STATE_2
	
	/* the data here is used for the shop_item_t's */
	uint32_t reserved2[0x18A];

#define SERIALIZABLE_DNR_STATE_3 \
	ADD_SERIALIZABLE(boolean32_t, is_hud_visible) \
	ADD_SERIALIZABLE_ARRAY(boolean32_t, seen_layer_messages, 14) \
	ADD_SERIALIZABLE(boolean32_t, seen_welcome_message) \
	ADD_SERIALIZABLE(boolean32_t, save_with_flag) \
	ADD_SERIALIZABLE(int16_t, spawn_x) \
	ADD_SERIALIZABLE(int16_t, spawn_y) \
	ADD_SERIALIZABLE(boolean32_t, show_flag) \
	ADD_SERIALIZABLE(boolean32_t, has_liquid_resistance) \
	ADD_SERIALIZABLE(shop_item_t, dirt_digger) \
	ADD_SERIALIZABLE(shop_item_t, rock_drill) \
	ADD_SERIALIZABLE(shop_item_t, stone_grinder) \
	ADD_SERIALIZABLE(shop_item_t, jump_upgrade) \
	ADD_SERIALIZABLE(shop_item_t, jetpack_upgrade) \
	ADD_SERIALIZABLE(shop_item_t, elements_resistance) \
	ADD_SERIALIZABLE(shop_item_t, scan_upgrade) \
	ADD_SERIALIZABLE(shop_item_t, vacpak_upgrade) \
	ADD_SERIALIZABLE(shop_item_t, wifi_upgrade) \
	ADD_SERIALIZABLE(shop_item_t, health_upgrade) \
	ADD_SERIALIZABLE(shop_item_t, battery_upgrade) \
	ADD_SERIALIZABLE(shop_item_t, dynamite) \
	ADD_SERIALIZABLE(shop_item_t, double_dynamite) \
	ADD_SERIALIZABLE(shop_item_t, mega_bomb) \
	ADD_SERIALIZABLE(shop_item_t, dirtzooka) \
	ADD_SERIALIZABLE(shop_item_t, dirtzooka_upgrade)
SERIALIZABLE_DNR_STATE_3
} dnr_state_t;

#define SERIALIZABLE_DNR_STATE SERIALIZABLE_DNR_STATE_0 SERIALIZABLE_DNR_STATE_1 SERIALIZABLE_DNR_STATE_2 SERIALIZABLE_DNR_STATE_3

#pragma pack()

#ifndef REMOVE_STATE_SIZE_CHECK
/* classic */
enum {
	DNR_STATE_T_SIZE_CHECK = 1 / (sizeof(dnr_state_t) == 23445008)
};
#endif

typedef enum asset_tile_type
{
#define SERIALIZABLE_ASSET_TILE_TYPE \
	ADD_SERIALIZABLE_ENUM(TILE_TYPE_DEFAULT, 0x0) \
	ADD_SERIALIZABLE_ENUM(TILE_TYPE_WATER, 0x1) \
	ADD_SERIALIZABLE_ENUM(TILE_TYPE_NO_MINERAL, 0x2) \
	ADD_SERIALIZABLE_ENUM(TILE_TYPE_CANNOT_MINE, 0x3) \
	ADD_SERIALIZABLE_ENUM(TILE_TYPE_LAVA, 0x4) \
	ADD_SERIALIZABLE_ENUM(TILE_TYPE_ENEMY_SPAWN, 0x5) \
	ADD_SERIALIZABLE_ENUM(TILE_TYPE_BACKGROUND, 0x6) \
	ADD_SERIALIZABLE_ENUM(TILE_TYPE_STALACTITE, 0x7) \
	ADD_SERIALIZABLE_ENUM(TILE_TYPE_ALUMON, 0x8) \
	ADD_SERIALIZABLE_ENUM(TILE_TYPE_ATOMICIDE, 0x9) \
	ADD_SERIALIZABLE_ENUM(TILE_TYPE_RADIOACTON, 0xA) \
	ADD_SERIALIZABLE_ENUM(TILE_TYPE_METALOID, 0xB) \
	ADD_SERIALIZABLE_ENUM(TILE_TYPE_EXPLODIUM, 0xC) \
	ADD_SERIALIZABLE_ENUM(TILE_TYPE_SCIPHIDE, 0xD) \
	ADD_SERIALIZABLE_ENUM(TILE_TYPE_ENERGITE, 0xE) \
	ADD_SERIALIZABLE_ENUM(TILE_TYPE_RAREIEST, 0xF)
SERIALIZABLE_ASSET_TILE_TYPE
} asset_tile_type_t;

typedef struct asset_block
{
#define SERIALIZABLE_ASSET_BLOCK \
	ADD_SERIALIZABLE(CHAR_INFO, visual) \
	ADD_SERIALIZABLE(asset_tile_type_t, tile_type) \
	ADD_SERIALIZABLE(boolean32_t, transparency)
SERIALIZABLE_ASSET_BLOCK
} asset_block_t;

typedef struct asset
{
#define SERIALIZABLE_ASSET \
	ADD_SERIALIZABLE(int32_t, width) \
	ADD_SERIALIZABLE(int32_t, height) \
	ADD_SERIALIZABLE(rgb_color_t, dirt_color) \
	ADD_SERIALIZABLE(dnr_weather_type_t, weather_type) \
	ADD_SERIALIZABLE(int32_t, weather_particle_rate) \
	ADD_SERIALIZABLE(float, weather_speed)
SERIALIZABLE_ASSET

	asset_block_t* blocks;
} asset_t;

#undef ADD_SERIALIZABLE
#undef ADD_SERIALIZABLE_ARRAY
#undef ADD_SERIALIZABLE_ENUM

typedef struct editor_state
{
	int max_events_per_frame;
	int simulation_framerate;
	bool is_small_console;
	int current_save;
	char current_asset_directory[MAX_PATH];
	char current_campaign_directory[MAX_PATH];
	info_mode_t current_mode;
	asset_block_t asset_palette[8];
} editor_state_t;

typedef struct layer
{
	const char* name;
	const char* directory;
} layer_t;

typedef struct campaign
{
	const char* name;
	int start_x, start_y;
	region_t end_box;
	layer_t layers[LAYER_COUNT];
} campaign_t;

bool file_editor_load(editor_state_t* state);
bool file_editor_save(const editor_state_t* state);

asset_t file_asset_load(const char* directory);
void file_asset_unload(asset_t* asset);
bool file_asset_save(const char* directory, const asset_t* asset);

campaign_t* file_campaign_blank(void);
campaign_t* file_campaign_load(const char* directory);
void file_campaign_unload(campaign_t* campaign);
bool file_campaign_save(const char* directory, const campaign_t* campaign);

dnr_state_t* file_state_load(const char* directory);
void file_state_unload(dnr_state_t* save);
bool file_state_save(const char* directory, const dnr_state_t* save);