/*
	layer_main.c ~ RL
*/

#include "layer_main.h"
#include "layer_info.h"
#include "../action_buffer.h"
#include "../info_box.h"
#include "../screen.h"
#include "../tool.h"
#include <stdio.h>

static char directory[MAX_PATH];
static asset_t layer;
static sprite_t cache;

static tool_brush_t tool_eraser;
static tool_brush_t tool_brush;
static tool_select_t tool_select;

static action_buffer_t action_buffer;

static region_t clipboard_region;
static asset_block_t* clipboard_data;

static editor_state_t* editor_state;

static void layer_invalidate(void)
{
	screen_sprite_destroy(cache);
	cache = game_spritify_asset(layer);
	RUNTIME_ASSERT(cache);
	screen_repaint();
}

static void layer_handle_file_change(const char* _directory)
{
	screen_sprite_destroy(cache);
	cache = NULL;
	file_asset_unload(&layer);

	snprintf(directory, sizeof directory, "%s", _directory);
	snprintf(editor_state->current_layer_directory, sizeof editor_state->current_layer_directory, "%s", _directory);
	layer = file_asset_load(directory);
	RUNTIME_ASSERT(layer.blocks);
	layer_info_asset_set(&layer);
	layer_invalidate();
}

static void layer_handle_block_change(region_t region)
{
	layer_invalidate();
}

static void layer_handle_tool_change(info_tool_t new_tool)
{
	tool_select_reset(tool_select);
	if (cache)
	{
		screen_repaint();
	}
}

static void layer_handle_brush_size_change(int new_size)
{
	tool_brush_set_size(tool_brush, new_size);
	tool_brush_set_size(tool_eraser, new_size);
}

static void layer_handle_global_field_change(const void* field)
{
	layer_invalidate();
}

static void layer_erase(tool_brush_t brush, region_t region)
{
	RUNTIME_ASSERT(brush == tool_eraser);

	asset_block_t* temp = dig_malloc(sizeof * temp * region_size(region));
	game_asset_copy(&layer, region, temp);

	game_asset_delete(&layer, region);

	for (int i = 0; i < region_size(region); i++)
	{
		tool_brush_add_to_before_list_ab(brush, &temp[i], region.x0 + i % region_width(region), region.y0 + i / region_width(region));
	}
	free(temp);
}

static void layer_brush(tool_brush_t brush, region_t region)
{
	RUNTIME_ASSERT(brush == tool_brush);

	asset_block_t* temp = dig_malloc(sizeof * temp * region_size(region));
	game_asset_copy(&layer, region, temp);

	asset_block_t block;
	layer_info_get_current_brush_block(&block);
	for (int y = 0; y < region_width(region); y++)
	{
		for (int x = 0; x < region_height(region); x++)
		{
			tool_brush_add_to_before_list_ab(brush, &temp[x + y * region_width(region)], x + region.x0, y + region.y0);
			game_asset_paste(&layer, (region_t) { x + region.x0, y + region.y0, x + region.x0, y + region.y0 }, & block);
		}
	}

	free(temp);
}

static inline void layer_render_tile_type_as(asset_tile_type_t type, char ch, attribute_t attrib)
{
	for (int y = 0; y < TARGET_HEIGHT; y++)
	{
		for (int x = 0; x < TARGET_WIDTH; x++)
		{
			if (layer.blocks[y * TARGET_WIDTH + x].tile_type == type)
			{
				screen_set_attrib_region(&attrib, (region_t) { x, y, x, y });
				screen_set_char_region(&ch, (region_t) { x, y, x, y });
			}
		}
	}
}

static void layer_handle_repaint(void)
{
	if (!cache)
	{
		return;
	}

	screen_change_dirt_color(screen_sprite_dirt_color(cache));
	screen_sprite_render(0, 0, cache);

	layer_render_tile_type_as(TILE_TYPE_ENEMY_SPAWN, 'X', CREATE_ATTRIBUTE(LIGHT_RED, DARK_BLACK));
	layer_render_tile_type_as(TILE_TYPE_LAVA, 'X', CREATE_ATTRIBUTE(DARK_RED, DARK_BLACK));
	layer_render_tile_type_as(TILE_TYPE_WATER, 'X', CREATE_ATTRIBUTE(DARK_BLUE, DARK_BLACK));

	tool_select_render(tool_select, 0);
}

static void layer_select_handle_mouse_button(bool m1_down, int x, int y)
{
	tool_event_t event = tool_select_handle_mouse_click(tool_select, m1_down, x, y, 0);
	switch (event)
	{
	case EVENT_SELECTION_MOVE_STOP:
	{
		region_t src = tool_select_region(tool_select);
		region_t dest = tool_select_move_region(tool_select);
		region_t total = region_merge(src, dest);

		action_buffer_pre_add_asset_block(action_buffer, &layer, total);

		asset_block_t* arr = dig_malloc(region_size(src) * sizeof * arr);
		game_asset_copy(&layer, src, arr);
		game_asset_delete(&layer, src);
		game_asset_paste(&layer, dest, arr);
		free(arr);

		action_buffer_post_add_asset_block(action_buffer, &layer);

		layer_invalidate();
		break;
	}
	case EVENT_SELECTION_RESIZE_STOP:
	{
		layer_info_asset_set_current(tool_select_region(tool_select));
		break;
	}
	}
	screen_repaint();
}

static void layer_brush_handle_mouse_button(tool_brush_t brush, bool m1_down, int x, int y)
{
	tool_event_t result = tool_brush_handle_mouse_click(brush, m1_down, x, y, 0);
	if (result == EVENT_BRUSH_END)
	{
		region_t region = tool_brush_region(brush);
		asset_block_t* temp = dig_malloc(region_size(region) * sizeof * temp * 2);

		tool_brush_copy_before_ab(brush, &layer, temp);
		game_asset_copy(&layer, region, temp + region_size(region));

		action_buffer_add_asset_block(action_buffer, temp, temp + region_size(region), region);

		free(temp);
	}
}

static void layer_brush_handle_mouse_move(tool_brush_t brush, bool m1_down, int x, int y)
{
	tool_event_t result = tool_brush_handle_mouse_move(brush, m1_down, x, y, 0);
	if (!region_is_invalid(tool_brush_region(brush)))
	{
		layer_invalidate();
	}
}

static void layer_handle_mouse_button(bool m1_down, int x, int y)
{
	info_tool_t current = layer_info_get_current_tool();
	if (current == TOOL_SELECT)
	{
		layer_select_handle_mouse_button(m1_down, x, y);
	}
	else if (current == TOOL_BRUSH)
	{
		layer_brush_handle_mouse_button(tool_brush, m1_down, x, y);
	}
	else if (current == TOOL_ERASER)
	{
		layer_brush_handle_mouse_button(tool_eraser, m1_down, x, y);
	}
}

static void layer_handle_mouse_move(bool m1_down, int x, int y)
{
	info_tool_t current = layer_info_get_current_tool();
	if (current == TOOL_SELECT)
	{
		tool_select_handle_mouse_move(tool_select, m1_down, x, y, 0);
		layer_invalidate();
	}
	else if (current == TOOL_BRUSH)
	{
		layer_brush_handle_mouse_move(tool_brush, m1_down, x, y);
	}
	else if (current == TOOL_ERASER)
	{
		layer_brush_handle_mouse_move(tool_eraser, m1_down, x, y);
	}
}

static void layer_do_action(action_t* act)
{
	if (!act)
	{
		return;
	}
	if (act->type == ACTION_FIELD)
	{
		return;
	}
	action_buffer_reverse_asset_block(&layer, act);
	layer_invalidate();
}

static void layer_copy(void)
{
	if (region_is_invalid(tool_select_region(tool_select)))
	{
		return;
	}

	if (clipboard_data)
	{
		free(clipboard_data);
	}

	clipboard_region = tool_select_region(tool_select);
	clipboard_data = dig_malloc(region_size(clipboard_region) * sizeof * clipboard_data);
	game_asset_copy(&layer, clipboard_region, clipboard_data);
}

static void layer_paste(void)
{
	region_t region = tool_select_region(tool_select);
	if (!clipboard_data || region_is_invalid(region))
	{
		return;
	}

	region_t dest = { region.x0, region.y0, region.x0 + region_width(clipboard_region) - 1, region.y0 + region_height(clipboard_region) - 1 };

	if (!dig_inside_bounds(dest.x0, dest.y0) || !dig_inside_bounds(dest.x1, dest.y1))
	{
		return;
	}

	action_buffer_pre_add_asset_block(action_buffer, &layer, dest);

	game_asset_delete(&layer, region);
	game_asset_paste(&layer, dest, clipboard_data);

	action_buffer_post_add_asset_block(action_buffer, &layer);

	tool_select_set_region(tool_select, dest);
	layer_invalidate();
}

static void layer_handle_keyboard(virtual_key_t key, keyboard_control_t ctrl)
{
	if (ctrl != CTRL_LEFT_PRESSED)
	{
		return;
	}

	switch (key)
	{
	case 'Z':
		layer_do_action(action_buffer_back(action_buffer));
		break;
	case 'Y':
		layer_do_action(action_buffer_forward(action_buffer));
		break;
	case 'S':
		debug_format("Saving to disk...\n");
		if (!file_asset_save(directory, &layer))
		{
			MessageBoxW(NULL, L"Failed to save file, maybe run in admin mode?", L"Dig-N-Rig Modder - Error!", MB_OK | MB_ICONERROR);
		}
		break;
	case 'C':
		layer_copy();
		break;
	case 'V':
		layer_paste();
		break;
	}
}

void layer_initialize(editor_state_t* state)
{
	editor_state = state;

	info_mode_class_t class =
	{
		.mode = MODE_LAYER,
		.caption = "Layer",
		.initialize = layer_info_initialize,
		.destroy = layer_info_destroy,
		.show = layer_info_show,
		.proc = layer_info_proc,
		.interact_tree_item = layer_info_handle_interact_tree_item
	};
	info_add_class(&class);

	tool_eraser = tool_brush_create(layer_erase, BRUSH_TYPE_ASSET_BLOCK, TARGET_WIDTH, TARGET_HEIGHT);
	tool_brush = tool_brush_create(layer_brush, BRUSH_TYPE_ASSET_BLOCK, TARGET_WIDTH, TARGET_HEIGHT);
	tool_select = tool_select_create(TARGET_WIDTH, TARGET_HEIGHT);

	action_buffer = action_buffer_initialize();
	layer_info_action_buffer_set(action_buffer);
}

void layer_destroy(void)
{
	file_asset_unload(&layer);
	screen_sprite_destroy(cache);
	tool_select_destroy(tool_select);
	action_buffer_destroy(action_buffer);
	free(clipboard_data);
}

void layer_start(void)
{
	info_events_t info_events =
	{
		.file_handler = layer_handle_file_change,
		.block_handler = layer_handle_block_change,
		.tool_handler = layer_handle_tool_change,
		.brush_size_handler = layer_handle_brush_size_change,
		.global_field_handler = layer_handle_global_field_change
	};
	info_set_event_handlers(&info_events);
	screen_events_t screen_events =
	{
		.repaint = layer_handle_repaint,
		.mouse_button = layer_handle_mouse_button,
		.mouse_move = layer_handle_mouse_move,
		.keyboard = layer_handle_keyboard,
	};
	screen_set_event_handlers(&screen_events);
	if (*editor_state->current_layer_directory)
	{
		layer_info_directory_set(editor_state->current_layer_directory);
		layer_handle_file_change(editor_state->current_layer_directory);
	}
}

void layer_end(void)
{
	tool_select_reset(tool_select);
}