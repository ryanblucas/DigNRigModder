/*
	layer_main.c ~ RL
*/

#include "layer_main.h"
#include "layer_info.h"
#include "../info_box.h"
#include "../screen.h"
#include "../tool.h"

static asset_t layer;
static sprite_t cache;

static tool_select_t tool_select;

void layer_initialize(editor_state_t* state)
{
	info_mode_class_t class =
	{
		.mode = MODE_LAYER,
		.caption = "Layer",
		.initialize = layer_info_initialize,
		.destroy = layer_info_destroy,
		.show = layer_info_show,
		.proc = layer_info_proc
	};
	info_add_class(&class);
	tool_select = tool_select_create(TARGET_WIDTH, TARGET_HEIGHT);
}

void layer_destroy(void)
{
	file_asset_unload(&layer);
	screen_sprite_destroy(cache);
	tool_select_destroy(tool_select);
}

static void layer_handle_file_change(const char* directory)
{
	screen_sprite_destroy(cache);
	file_asset_unload(&layer);

	layer = file_asset_load(directory);
	RUNTIME_ASSERT(layer.blocks);
	layer_info_asset_set(&layer);
	cache = game_spritify_asset(layer);
	RUNTIME_ASSERT(cache);

	screen_repaint();
}

static void layer_handle_repaint(void)
{
	if (!cache)
	{
		return;
	}

	screen_change_dirt_color(screen_sprite_dirt_color(cache));
	screen_sprite_render(0, 0, cache);
	tool_select_render(tool_select, 0);
}

static void layer_handle_mouse_button(bool m1_down, int x, int y)
{
	tool_event_t event = tool_select_handle_mouse_click(tool_select, m1_down, x, y, 0);
	if (event == EVENT_SELECTION_MOVE_STOP)
	{
		region_t src = tool_select_region(tool_select);
		region_t dest = tool_select_move_region(tool_select);

		asset_block_t* arr = dig_malloc(region_size(src) * sizeof * arr);
		game_asset_copy(&layer, src, arr);
		game_asset_delete(&layer, src);
		game_asset_paste(&layer, dest, arr);
		free(arr);

		cache = game_spritify_asset(layer);
	}
	screen_repaint();
}

static void layer_handle_mouse_move(bool m1_down, int x, int y)
{
	tool_select_handle_mouse_move(tool_select, m1_down, x, y, 0);
	screen_repaint();
}

void layer_start(void)
{
	info_events_t info_events =
	{
		.file_handler = layer_handle_file_change
	};
	info_set_event_handlers(&info_events);
	screen_events_t screen_events =
	{
		.repaint = layer_handle_repaint,
		.mouse_button = layer_handle_mouse_button,
		.mouse_move = layer_handle_mouse_move,
	};
	screen_set_event_handlers(&screen_events);
}

void layer_end(void)
{
	tool_select_reset(tool_select);
}