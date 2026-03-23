/*
	layer_main.c ~ RL
*/

#include "layer_main.h"
#include "layer_info.h"
#include "../info_box.h"
#include "../screen.h"

#define MAX_SELECTION_WIDTH 120
#define MAX_SELECTION_HEIGHT 80
#define MAX_SELECTION_SIZE (MAX_SELECTION_WIDTH * MAX_SELECTION_HEIGHT)

static sprite_t cache;

static sprite_t selection_visual;
static region_t selection_region;
static int hinge_x = -1, hinge_y = -1;
static int move_x = -1, move_y = -1;

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
}

void layer_destroy(void)
{
	screen_sprite_destroy(cache);
}

static void layer_handle_file_change(const char* directory)
{
	screen_sprite_destroy(cache);
	asset_t asset = file_asset_load(directory);
	cache = game_spritify_asset(asset);
	file_asset_unload(&asset);
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
	if (hinge_x >= 0 && hinge_y >= 0)
	{
		region_t screen_region = region_validate(selection_region);

		attribute_t* selected = dig_malloc(region_size(screen_region) * sizeof * selected);
		memset(selected, 0, region_size(screen_region) * sizeof * selected);

		screen_set_attrib_region(selected, screen_region);
		screen_sprite_render(move_x - hinge_x, move_y - hinge_y, selection_visual);
		free(selected);
		return;
	}
	screen_invert_region(region_validate(selection_region));
}

static void layer_start_move(int new_selected_x, int new_selected_y)
{
	region_t correct = region_validate(selection_region);
	hinge_x = new_selected_x - correct.x0;
	hinge_y = new_selected_y - correct.y0;
	move_x = new_selected_x;
	move_y = new_selected_y;

	char* text = dig_malloc(region_size(selection_region));
	attribute_t* attributes = dig_malloc(region_size(selection_region) * sizeof * attributes);

	screen_get_char_region(text, correct);
	screen_get_attrib_region(attributes, correct);

	/* b/c it's selected rn */
	for (int i = 0; i < region_size(selection_region); i++)
	{
		attributes[i] = ~attributes[i] & 0xFF;
	}

	selection_visual = screen_sprite_create(region_width(selection_region), region_height(selection_region), 0, text, attributes);

	free(text);
	free(attributes);

	screen_repaint();
}

static void layer_stop_move(void)
{
	region_t src_region = region_validate(selection_region);
	region_t dest_region =
	{
		move_x - hinge_x,
		move_y - hinge_y,
		move_x - hinge_x + region_width(src_region) - 1,
		move_y - hinge_y + region_height(src_region) - 1
	};

	region_t total = region_merge(src_region, dest_region);

	if (memcmp(&src_region, &dest_region, sizeof src_region) != 0)
	{

	}

	hinge_x = hinge_y = -1;
	move_x = move_y = -1;
	selection_region = INVALID_REGION;
	screen_sprite_destroy(selection_visual);
	selection_visual = NULL;

	screen_repaint();
}

static void layer_select_handle_mouse_button(bool m1_down, int x, int y)
{
	if (!m1_down)
	{
		if (hinge_x >= 0 && hinge_y >= 0)
		{
			layer_stop_move();
		}
		if (!region_is_invalid(selection_region) && region_size(selection_region) > 1)
		{
			//save_info_cell_set_current_region(selection_region);
			screen_repaint();
		}
		return;
	}

	if (!region_is_invalid(selection_region) && region_is_inside(selection_region, x, y))
	{
		layer_start_move(x, y);
		return;
	}

	selection_region.x0 = selection_region.x1 = x;
	selection_region.y0 = selection_region.y1 = y;

	screen_repaint();
}

static void layer_select_handle_mouse_move(bool m1_down, int x, int y)
{
	if (!m1_down || region_is_invalid(selection_region))
	{
		return;
	}

	if (hinge_x >= 0 && hinge_y >= 0)
	{
		move_x = x;
		move_y = y;
		screen_repaint();
		return;
	}

	selection_region.x1 = min(x, selection_region.x0 + MAX_SELECTION_WIDTH - 1);
	selection_region.y1 = min(y, selection_region.y0 + MAX_SELECTION_HEIGHT - 1);
	selection_region.x1 = max(selection_region.x1, selection_region.x0 - MAX_SELECTION_WIDTH + 1);
	selection_region.y1 = max(selection_region.y1, selection_region.y0 - MAX_SELECTION_HEIGHT + 1);
	if (region_size(selection_region) != 1)
	{
		//save_info_cell_set_current(-1, -1);
	}
	screen_repaint();
}

static void layer_handle_mouse_button(bool m1_down, int x, int y)
{
	layer_select_handle_mouse_button(m1_down, x, y);
}

static void layer_handle_mouse_move(bool m1_down, int x, int y)
{
	layer_select_handle_mouse_move(m1_down, x, y);
}

void layer_start(void)
{
	selection_region = INVALID_REGION;
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
	selection_region = INVALID_REGION;
}