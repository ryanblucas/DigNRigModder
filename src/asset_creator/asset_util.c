/*
	asset_util.c ~ RL
*/

#include "asset_util.h"
#include "../game.h"
#include "../interface/mineral_palette.h"

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

tool_event_t asset_select_handle_mouse_button(asset_suite_t* suite, bool m1_down, int x, int y)
{
	tool_event_t event = tool_select_handle_mouse_click(suite->tool_select, m1_down, x, y, -suite->scroll_x, -suite->scroll_y);
	if (event == EVENT_SELECTION_MOVE_STOP)
	{
		region_t src = tool_select_region(suite->tool_select);
		region_t dest = tool_select_move_region(suite->tool_select);
		region_t total = region_keep_inside(region_merge(src, dest), (region_t) { .x1 = suite->asset->width - 1, .y1 = suite->asset->height - 1 });

		action_buffer_pre_add_asset_block(suite->buffer, suite->asset, total);

		asset_block_t* arr = dig_malloc(region_size(src) * sizeof * arr);
		game_asset_copy(suite->asset, src, arr);
		game_asset_delete(suite->asset, src);
		game_asset_paste(suite->asset, dest, arr);
		free(arr);

		action_buffer_post_add_asset_block(suite->buffer, suite->asset);
	}
	screen_repaint();
	return event;
}

tool_event_t asset_brush_handle_mouse_button(asset_suite_t* suite, bool is_eraser, bool m1_down, int x, int y)
{
	x -= suite->scroll_x;
	y -= suite->scroll_y;
	tool_brush_t brush = is_eraser ? suite->tool_eraser : suite->tool_brush;
	tool_event_t result = tool_brush_handle_mouse_click(brush, m1_down, x, y, 0);
	if (result == EVENT_BRUSH_END)
	{
		region_t region = tool_brush_region(brush);
		asset_block_t* temp = dig_malloc(region_size(region) * sizeof * temp * 2);

		tool_brush_copy_before_ab(brush, suite->asset, temp);
		game_asset_copy(suite->asset, region, temp + region_size(region));

		action_buffer_add_asset_block(suite->buffer, temp, temp + region_size(region), region);

		free(temp);
	}
	return result;
}

void asset_handle_brush(asset_suite_t* suite, region_t region, asset_block_t block)
{
	asset_block_t* temp = dig_malloc(sizeof * temp * region_size(region));
	game_asset_copy(suite->asset, region, temp);

	for (int y = 0; y < region_width(region); y++)
	{
		for (int x = 0; x < region_height(region); x++)
		{
			tool_brush_add_to_before_list_ab(suite->tool_brush, &temp[x + y * region_width(region)], x + region.x0, y + region.y0);
			game_asset_paste(suite->asset, (region_t) { x + region.x0, y + region.y0, x + region.x0, y + region.y0 }, & block);
		}
	}

	free(temp);
}

void asset_handle_erase(asset_suite_t* suite, region_t region)
{
	asset_block_t* temp = dig_malloc(sizeof * temp * region_size(region));
	game_asset_copy(suite->asset, region, temp);
	game_asset_delete(suite->asset, region);
	for (int i = 0; i < region_size(region); i++)
	{
		tool_brush_add_to_before_list_ab(suite->tool_eraser, &temp[i], region.x0 + i % region_width(region), region.y0 + i / region_width(region));
	}
	free(temp);
}

void asset_handle_copy(asset_suite_t* suite)
{
	if (region_is_invalid(tool_select_region(suite->tool_select)))
	{
		return;
	}

	if (suite->clipboard_data)
	{
		free(suite->clipboard_data);
	}

	suite->clipboard_region = tool_select_region(suite->tool_select);
	suite->clipboard_data = dig_malloc(region_size(suite->clipboard_region) * sizeof * suite->clipboard_data);
	game_asset_copy(suite->asset, suite->clipboard_region, suite->clipboard_data);
}

void asset_handle_paste(asset_suite_t* suite)
{
	region_t region = tool_select_region(suite->tool_select);
	if (!suite->clipboard_data || region_is_invalid(region))
	{
		return;
	}

	region_t dest = { region.x0, region.y0, region.x0 + region_width(suite->clipboard_region) - 1, region.y0 + region_height(suite->clipboard_region) - 1 };
	region_t asset_region = { 0, 0, suite->asset->width - 1, suite->asset->height - 1 };
	if (!region_is_inside(asset_region, dest.x0, dest.y0) || !region_is_inside(asset_region, dest.x1, dest.y1))
	{
		return;
	}

	action_buffer_pre_add_asset_block(suite->buffer, suite->asset, dest);

	game_asset_delete(suite->asset, region);
	game_asset_paste(suite->asset, dest, suite->clipboard_data);

	action_buffer_post_add_asset_block(suite->buffer, suite->asset);

	tool_select_set_region(suite->tool_select, dest);
}

void asset_set_current_treeview(HWND treeview, asset_block_t* block, asset_info_current_field_t settings)
{
	TreeView_DeleteAllItems(treeview);
#define ADD_SERIALIZABLE(type, name) element_t name = serialize_single(#type, &block->name, #name, treeview, NULL);
#define ADD_SERIALIZABLE_ARRAY(type, name, count) element_t name = serialize_array(#type, &block->name, count, #name, treeview, NULL);
	SERIALIZABLE_ASSET_BLOCK
#undef ADD_SERIALIZABLE
#undef ADD_SERIALIZABLE_ARRAY

	if (settings & AICF_TILE_TYPE)
	{
		serialize_element_enable(tile_type, false);
	}
	if (settings & AICF_VISUAL)
	{
		serialize_element_enable(visual, false);
	}
	if (settings & AICF_TRANSPARENCY)
	{
		serialize_element_enable(transparency, false);
	}
}

void asset_set_current_treeview_from_region(HWND treeview, asset_t* asset, region_t region, asset_block_t* result)
{
	if (region_is_invalid(region))
	{
		TreeView_DeleteAllItems(treeview);
		return;
	}
	region = region_validate(region);
	*result = asset->blocks[region.x0 + region.y0 * asset->width];
	enum asset_info_current_field mask = 0;
	for (int y = 0; y < region_height(region); y++)
	{
		for (int x = 0; x < region_width(region); x++)
		{
			asset_block_t* curr = &asset->blocks[(region.x0 + x) + (region.y0 + y) * asset->width];
			if (~mask & AICF_TILE_TYPE && result->tile_type != curr->tile_type)
			{
				result->tile_type = 0;
				mask |= AICF_TILE_TYPE;
			}
			else if (~mask & AICF_VISUAL && result->visual.Attributes != curr->visual.Attributes || result->visual.Char.AsciiChar != curr->visual.Char.AsciiChar)
			{
				result->visual = (CHAR_INFO){ 0 };
				mask |= AICF_VISUAL;
			}
			else if (~mask & AICF_TILE_TYPE && result->transparency != curr->transparency)
			{
				result->transparency = false;
				mask |= AICF_TRANSPARENCY;
			}
		}
	}

	asset_set_current_treeview(treeview, result, mask);
}

bool asset_handle_interact_treeview(asset_info_suite_t* suite, bool is_global, element_t element)
{
	/* only should change elementary fields */
	if (serialize_element_get_size(element) > 4 || serialize_element_get_count(element) > 1)
	{
		return true;
	}
	if (is_global)
	{
		field_t begin_copy = field_create(serialize_element_get_value(element), serialize_element_get_size(element));
		if (!serialize_on_change_field(element))
		{
			return false;
		}
		if (begin_copy != field_create(serialize_element_get_value(element), serialize_element_get_size(element)))
		{
			queue_add(suite->internal.events->global_field_handler, serialize_element_get_value(element));
			action_buffer_add_field(suite->action_buffer, element, begin_copy);
		}
		return true;
	}

	field_t previous = field_create(serialize_element_get_value(element), serialize_element_get_size(element));
	if (!serialize_on_change_field(element))
	{
		return false;
	}
	if (previous == field_create(serialize_element_get_value(element), serialize_element_get_size(element)))
	{
		return true;
	}
	if (suite->current_tool == TOOL_BRUSH && MINERAL_PALETTE_GET_SELECTED_CELL(suite->palette_window) != -1)
	{
		MINERAL_PALETTE_SET_CELL(suite->palette_window, MINERAL_PALETTE_GET_SELECTED_CELL(suite->palette_window), &suite->tool_blocks[TOOL_BRUSH]);
		return true;
	}
	if (region_is_invalid(suite->selection_region))
	{
		return true;
	}
	action_buffer_pre_add_asset_block(suite->action_buffer, suite->asset, suite->selection_region);
	for (int y = suite->selection_region.y0; y <= suite->selection_region.y1; y++)
	{
		for (int x = suite->selection_region.x0; x <= suite->selection_region.x1; x++)
		{
			uint8_t* current = (uint8_t*)serialize_element_get_value(element);
			memcpy((uint8_t*)&suite->asset->blocks[x + y * suite->asset->width] + (current - (uint8_t*)&suite->tool_blocks[suite->current_tool]), current, serialize_element_get_size(element));
		}
	}
	queue_add(suite->internal.events->block_handler, &suite->selection_region);
	action_buffer_post_add_asset_block(suite->action_buffer, suite->asset);
	return true;
}