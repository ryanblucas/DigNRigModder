/*
	asset_main.c ~ RL
*/

#include "asset_main.h"
#include "asset_info.h"
#include "../action_buffer.h"
#include "../info_box.h"
#include "../screen.h"
#include "../tool.h"
#include "../weather.h"
#include <stdio.h>

static void asset_brush(tool_brush_t brush, region_t region);
static void asset_erase(tool_brush_t brush, region_t region);

static char directory[MAX_PATH];
static asset_t asset;
static bool is_layer;
static sprite_t cache;

static tool_brush_t tool_eraser;
static tool_brush_t tool_brush;
static tool_select_t tool_select;

static action_buffer_t action_buffer;

static region_t clipboard_region;
static asset_block_t* clipboard_data;

static editor_state_t* editor_state;

static int scroll_x, scroll_y;

static void asset_invalidate(void)
{
	screen_sprite_destroy(cache);
	cache = game_spritify_asset(asset);
	RUNTIME_ASSERT(cache);
	screen_repaint();
}

static void asset_handle_file_change(const char* _directory)
{
	screen_clear();

	screen_sprite_destroy(cache);
	cache = NULL;
	file_asset_unload(&asset);

	snprintf(directory, sizeof directory, "%s", _directory);
	snprintf(editor_state->current_asset_directory, sizeof editor_state->current_asset_directory, "%s", _directory);
	asset = file_asset_load(directory);

	is_layer = false;
	char* end = strrchr(directory, '.');
	if (end && strncmp(end, ".layer", 6) == 0)
	{
		is_layer = true;
	}

	RUNTIME_ASSERT(asset.blocks);
	asset_info_set(&asset);

	scroll_x = TARGET_WIDTH / 2 - asset.width / 2;
	scroll_y = TARGET_HEIGHT / 2 - asset.height / 2;

	asset_invalidate();

	tool_brush_destroy(tool_eraser);
	tool_brush_destroy(tool_brush);
	tool_select_destroy(tool_select);

	tool_eraser = tool_brush_create(asset_erase, BRUSH_TYPE_ASSET_BLOCK, asset.width, asset.height);
	tool_brush = tool_brush_create(asset_brush, BRUSH_TYPE_ASSET_BLOCK, asset.width, asset.height);
	tool_select = tool_select_create(asset.width, asset.height);

	weather_force_end();
	weather_start(asset.weather_type, asset.weather_particle_rate, asset.weather_speed);
}

static void asset_handle_block_change(region_t region)
{
	asset_invalidate();
}

static void asset_handle_tool_change(info_tool_t new_tool)
{
	tool_select_reset(tool_select);
	if (cache)
	{
		screen_repaint();
	}
}

static void asset_handle_brush_size_change(int new_size)
{
	tool_brush_set_size(tool_brush, new_size);
	tool_brush_set_size(tool_eraser, new_size);
}

static void asset_handle_global_field_change(const void* field)
{
	if (field == &asset.weather_type || field == &asset.weather_particle_rate || field == &asset.weather_speed)
	{
		weather_force_end();
		weather_start(asset.weather_type, asset.weather_particle_rate, asset.weather_speed);
	}
	asset_invalidate();
}

static void asset_erase(tool_brush_t brush, region_t region)
{
	RUNTIME_ASSERT(brush == tool_eraser);

	asset_block_t* temp = dig_malloc(sizeof * temp * region_size(region));
	game_asset_copy(&asset, region, temp);

	game_asset_delete(&asset, region);

	for (int i = 0; i < region_size(region); i++)
	{
		tool_brush_add_to_before_list_ab(brush, &temp[i], region.x0 + i % region_width(region), region.y0 + i / region_width(region));
	}
	free(temp);
}

static void asset_brush(tool_brush_t brush, region_t region)
{
	RUNTIME_ASSERT(brush == tool_brush);

	asset_block_t* temp = dig_malloc(sizeof * temp * region_size(region));
	game_asset_copy(&asset, region, temp);

	asset_block_t block;
	asset_info_get_current_brush_block(&block);
	for (int y = 0; y < region_width(region); y++)
	{
		for (int x = 0; x < region_height(region); x++)
		{
			tool_brush_add_to_before_list_ab(brush, &temp[x + y * region_width(region)], x + region.x0, y + region.y0);
			game_asset_paste(&asset, (region_t) { x + region.x0, y + region.y0, x + region.x0, y + region.y0 }, & block);
		}
	}

	free(temp);
}

static void asset_render_tile_type_as(asset_tile_type_t type, char ch, attribute_t attrib)
{
	for (int y = 0; y < asset.height; y++)
	{
		for (int x = 0; x < asset.width; x++)
		{
			asset_block_t* block = &asset.blocks[y * asset.width + x];
			if (block->tile_type == type && (is_layer || block->transparency))
			{
				screen_set_attrib_region(&attrib, (region_t) { scroll_x + x, scroll_y + y, scroll_x + x, scroll_y + y });
				screen_set_char_region(&ch, (region_t) { scroll_x + x, scroll_y + y, scroll_x + x, scroll_y + y });
			}
		}
	}
}

static void asset_handle_repaint(void)
{
	if (!cache)
	{
		return;
	}

	screen_change_dirt_color(screen_sprite_dirt_color(cache));
	screen_sprite_render(scroll_x, scroll_y, cache);

	asset_render_tile_type_as(TILE_TYPE_ENEMY_SPAWN, 'X', CREATE_ATTRIBUTE(LIGHT_RED, DARK_BLACK));
	asset_render_tile_type_as(TILE_TYPE_LAVA, 'X', CREATE_ATTRIBUTE(DARK_RED, DARK_BLACK));
	asset_render_tile_type_as(TILE_TYPE_WATER, 'X', CREATE_ATTRIBUTE(DARK_BLUE, DARK_BLACK));
	asset_render_tile_type_as(TILE_TYPE_STALACTITE, 0x1F, CREATE_ATTRIBUTE(DARK_YELLOW, DARK_BLACK));

	tool_select_render(tool_select, scroll_x, scroll_y);
	char ch = ' ';
	attribute_t attrib = 0;
	for (int y = 0; y < TARGET_HEIGHT; y++)
	{
		for (int x = 0; x < TARGET_WIDTH; x++)
		{
			if (x < scroll_x || x >= scroll_x + asset.width || y < scroll_y || y >= scroll_y + asset.height)
			{
				screen_set_attrib_region(&attrib, (region_t) { x, y, x, y });
				screen_set_char_region(&ch, (region_t) { x, y, x, y });
			}
		}
	}
}

static void asset_select_handle_mouse_button(bool m1_down, int x, int y)
{
	tool_event_t event = tool_select_handle_mouse_click(tool_select, m1_down, x, y, 0);
	switch (event)
	{
	case EVENT_SELECTION_MOVE_STOP:
	{
		region_t src = tool_select_region(tool_select);
		region_t dest = tool_select_move_region(tool_select);
		region_t total = region_keep_inside(region_merge(src, dest), (region_t){ .x1 = asset.width - 1, .y1 = asset.height - 1 });

		action_buffer_pre_add_asset_block(action_buffer, &asset, total);

		asset_block_t* arr = dig_malloc(region_size(src) * sizeof * arr);
		game_asset_copy(&asset, src, arr);
		game_asset_delete(&asset, src);
		game_asset_paste(&asset, dest, arr);
		free(arr);

		action_buffer_post_add_asset_block(action_buffer, &asset);

		asset_invalidate();
		break;
	}
	case EVENT_SELECTION_RESIZE_STOP:
	{
		asset_info_set_current(tool_select_region(tool_select));
		break;
	}
	}
	screen_repaint();
}

static void asset_brush_handle_mouse_button(tool_brush_t brush, bool m1_down, int x, int y)
{
	tool_event_t result = tool_brush_handle_mouse_click(brush, m1_down, x, y, 0);
	if (result == EVENT_BRUSH_END)
	{
		region_t region = tool_brush_region(brush);
		asset_block_t* temp = dig_malloc(region_size(region) * sizeof * temp * 2);

		tool_brush_copy_before_ab(brush, &asset, temp);
		game_asset_copy(&asset, region, temp + region_size(region));

		action_buffer_add_asset_block(action_buffer, temp, temp + region_size(region), region);

		free(temp);
	}
}

static void asset_brush_handle_mouse_move(tool_brush_t brush, bool m1_down, int x, int y)
{
	tool_event_t result = tool_brush_handle_mouse_move(brush, m1_down, x, y, 0);
	if (!region_is_invalid(tool_brush_region(brush)))
	{
		asset_invalidate();
	}
}

static void asset_handle_mouse_button(bool m1_down, int x, int y)
{
	x -= scroll_x;
	y -= scroll_y;
	if (x >= asset.width || y >= asset.height || x < 0 || y < 0)
	{
		return;
	}
	info_tool_t current = asset_info_get_current_tool();
	if (current == TOOL_SELECT)
	{
		asset_select_handle_mouse_button(m1_down, x, y);
	}
	else if (current == TOOL_BRUSH)
	{
		asset_brush_handle_mouse_button(tool_brush, m1_down, x, y);
	}
	else if (current == TOOL_ERASER)
	{
		asset_brush_handle_mouse_button(tool_eraser, m1_down, x, y);
	}
}

static void asset_handle_mouse_move(bool m1_down, int x, int y)
{
	x -= scroll_x;
	y -= scroll_y;
	if (x >= asset.width || y >= asset.height || x < 0 || y < 0)
	{
		return;
	}
	info_tool_t current = asset_info_get_current_tool();
	if (current == TOOL_SELECT)
	{
		tool_select_handle_mouse_move(tool_select, m1_down, x, y, 0);
		asset_invalidate();
	}
	else if (current == TOOL_BRUSH)
	{
		asset_brush_handle_mouse_move(tool_brush, m1_down, x, y);
	}
	else if (current == TOOL_ERASER)
	{
		asset_brush_handle_mouse_move(tool_eraser, m1_down, x, y);
	}
}

static void asset_do_action(action_t* act)
{
	if (!act)
	{
		return;
	}
	if (act->type == ACTION_FIELD)
	{
		return;
	}
	action_buffer_reverse_asset_block(&asset, act);
	asset_invalidate();
}

static void asset_copy(void)
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
	game_asset_copy(&asset, clipboard_region, clipboard_data);
}

static void asset_paste(void)
{
	region_t region = tool_select_region(tool_select);
	if (!clipboard_data || region_is_invalid(region))
	{
		return;
	}

	region_t dest = { region.x0, region.y0, region.x0 + region_width(clipboard_region) - 1, region.y0 + region_height(clipboard_region) - 1 };
	region_t asset_region = { 0, 0, asset.width - 1, asset.height - 1 };
	if (!region_is_inside(asset_region, dest.x0, dest.y0) || !region_is_inside(asset_region, dest.x1, dest.y1))
	{
		return;
	}

	action_buffer_pre_add_asset_block(action_buffer, &asset, dest);

	game_asset_delete(&asset, region);
	game_asset_paste(&asset, dest, clipboard_data);

	action_buffer_post_add_asset_block(action_buffer, &asset);

	tool_select_set_region(tool_select, dest);
	asset_invalidate();
}

static void asset_handle_keyboard(virtual_key_t key, keyboard_control_t ctrl)
{
	if (ctrl != CTRL_LEFT_PRESSED)
	{
		return;
	}

	switch (key)
	{
	case 'Z':
		asset_do_action(action_buffer_back(action_buffer));
		break;
	case 'Y':
		asset_do_action(action_buffer_forward(action_buffer));
		break;
	case 'S':
		debug_format("Saving to disk...\n");
		if (!file_asset_save(directory, &asset))
		{
			MessageBoxW(NULL, L"Failed to save file, maybe run in admin mode?", L"Dig-N-Rig Modder - Error!", MB_OK | MB_ICONERROR);
		}
		break;
	case 'C':
		asset_copy();
		break;
	case 'V':
		asset_paste();
		break;
	}
}

void asset_initialize(editor_state_t* state)
{
	editor_state = state;

	info_mode_class_t class =
	{
		.mode = MODE_ASSET,
		.caption = "Asset",
		.initialize = asset_info_initialize,
		.destroy = asset_info_destroy,
		.show = asset_info_show,
		.proc = asset_info_proc,
		.interact_tree_item = asset_info_handle_interact_tree_item
	};
	info_add_class(&class);

	tool_eraser = tool_brush_create(asset_erase, BRUSH_TYPE_ASSET_BLOCK, TARGET_WIDTH, TARGET_HEIGHT);
	tool_brush = tool_brush_create(asset_brush, BRUSH_TYPE_ASSET_BLOCK, TARGET_WIDTH, TARGET_HEIGHT);
	tool_select = tool_select_create(TARGET_WIDTH, TARGET_HEIGHT);

	action_buffer = action_buffer_initialize();
	asset_info_action_buffer_set(action_buffer);
}

void asset_destroy(void)
{
	file_asset_unload(&asset);
	screen_sprite_destroy(cache);
	tool_select_destroy(tool_select);
	action_buffer_destroy(action_buffer);
	free(clipboard_data);
}

void asset_start(void)
{
	info_events_t info_events =
	{
		.file_handler = asset_handle_file_change,
		.block_handler = asset_handle_block_change,
		.tool_handler = asset_handle_tool_change,
		.brush_size_handler = asset_handle_brush_size_change,
		.global_field_handler = asset_handle_global_field_change
	};
	info_set_event_handlers(&info_events);
	screen_simulator_t simulators[] = { weather_simulate, NULL };
	screen_events_t screen_events =
	{
		.repaint = asset_handle_repaint,
		.mouse_button = asset_handle_mouse_button,
		.mouse_move = asset_handle_mouse_move,
		.keyboard = asset_handle_keyboard,
		.simulators = simulators
	};
	screen_set_event_handlers(&screen_events);
	if (*editor_state->current_asset_directory)
	{
		asset_info_directory_set(editor_state->current_asset_directory);
		asset_handle_file_change(editor_state->current_asset_directory);
	}
	asset_info_palette_copy(editor_state->asset_palette, sizeof editor_state->asset_palette / sizeof * editor_state->asset_palette);
}

void asset_end(void)
{
	weather_force_end();
	tool_select_reset(tool_select);
	asset_info_palette_save(editor_state->asset_palette, sizeof editor_state->asset_palette / sizeof * editor_state->asset_palette);
}