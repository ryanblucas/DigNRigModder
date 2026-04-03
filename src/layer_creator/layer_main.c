/*
	layer_main.c ~ RL
*/

#include "layer_main.h"
#include "layer_info.h"
#include "../action_buffer.h"
#include "../info_box.h"
#include "../screen.h"
#include "../tool.h"

static asset_t layer;
static sprite_t cache;

static tool_select_t tool_select;

static action_buffer_t action_buffer;

static void layer_invalidate(void)
{
	screen_sprite_destroy(cache);
	cache = game_spritify_asset(layer);
	RUNTIME_ASSERT(cache);
	screen_repaint();
}

static void layer_handle_file_change(const char* directory)
{
	screen_sprite_destroy(cache);
	file_asset_unload(&layer);

	layer = file_asset_load(directory);
	RUNTIME_ASSERT(layer.blocks);
	layer_info_asset_set(&layer);
	layer_invalidate();
}

static void layer_handle_block_change(region_t region)
{
	layer_invalidate();
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

static void layer_handle_mouse_button(bool m1_down, int x, int y)
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

static void layer_handle_mouse_move(bool m1_down, int x, int y)
{
	tool_select_handle_mouse_move(tool_select, m1_down, x, y, 0);
	screen_repaint();
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

static void layer_handle_keyboard(virtual_key_t key, keyboard_control_t ctrl)
{
	if (ctrl == CTRL_LEFT_PRESSED && key == 'Z')
	{
		layer_do_action(action_buffer_back(action_buffer));
	}
	else if (ctrl == CTRL_LEFT_PRESSED && key == 'Y')
	{
		layer_do_action(action_buffer_forward(action_buffer));
	}
}

void layer_initialize(editor_state_t* state)
{
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
}

void layer_start(void)
{
	info_events_t info_events =
	{
		.file_handler = layer_handle_file_change,
		.block_handler = layer_handle_block_change,
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
}

void layer_end(void)
{
	tool_select_reset(tool_select);
}