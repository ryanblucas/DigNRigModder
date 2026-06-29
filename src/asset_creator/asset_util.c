/*
	asset_util.c ~ RL
*/

#include "asset_util.h"
#include "../game.h"

static void asset_render_tile_type_as(const asset_t* asset, bool is_layer, asset_tile_type_t type, char ch, attribute_t attrib, int sx, int sy)
{
	for (int y = 0; y < asset->height; y++)
	{
		for (int x = 0; x < asset->width; x++)
		{
			asset_block_t* block = &asset->blocks[y * asset->width + x];
			if (block->tile_type == type && (is_layer || block->transparency))
			{
				screen_set_char_region(&ch, (region_t) { sx + x, sy + y, sx + x, sy + y });
				screen_set_attrib_region(&attrib, (region_t) { sx + x, sy + y, sx + x, sy + y });
			}
		}
	}
}

void asset_render(const asset_t* asset, bool is_layer, int x, int y)
{
	sprite_t spr = game_spritify_asset(*asset);

	screen_sprite_render(x, y, spr);
	asset_render_tile_type_as(asset, is_layer, TILE_TYPE_ENEMY_SPAWN, 'X', CREATE_ATTRIBUTE(LIGHT_RED, DARK_BLACK), x, y);
	asset_render_tile_type_as(asset, is_layer, TILE_TYPE_LAVA, 'X', CREATE_ATTRIBUTE(DARK_RED, DARK_BLACK), x, y);
	asset_render_tile_type_as(asset, is_layer, TILE_TYPE_WATER, 'X', CREATE_ATTRIBUTE(DARK_BLUE, DARK_BLACK), x, y);
	asset_render_tile_type_as(asset, is_layer, TILE_TYPE_STALACTITE, 0x1F, CREATE_ATTRIBUTE(DARK_YELLOW, DARK_BLACK), x, y);

	screen_sprite_destroy(spr);
}