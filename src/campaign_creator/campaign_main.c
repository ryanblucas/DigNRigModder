/*
	campaign_main.c ~ RL
*/

#include "campaign_main.h"
#include "campaign_info.h"
#include "../info_box.h"
#include "../path.h"
#include "../screen.h"
#include <stdio.h>

static editor_state_t* editor_state;
static campaign_t* current_campaign;
static int y_pos;
static sprite_t cache[14];
static asset_t layers[14];

static bool campaign_try_load(const char* directory)
{
	file_campaign_unload(current_campaign);
	bool result = false;
	if (path_exists(directory))
	{
		current_campaign = file_campaign_load(directory);
		result = !!current_campaign;
		strncpy(editor_state->current_campaign_directory, directory, result ? sizeof editor_state->current_campaign_directory : 1);
	}
	if (!result)
	{
		debug_format("Failed to load campaign file (%s).\n", directory);
		current_campaign = file_campaign_blank();
	}
	campaign_info_set(current_campaign, editor_state->current_campaign_directory);
	for (int i = 0; i < 14; i++)
	{
		char sub_buf[MAX_PATH], abs_buf[MAX_PATH];
		snprintf(sub_buf, sizeof sub_buf, "Layers\\%s", current_campaign->layers[i].directory);
		layers[i] = file_asset_load(path_find_dnr_main(abs_buf, sizeof abs_buf, sub_buf));
		cache[i] = game_spritify_asset(layers[i]);
	}
	return result;
}

void campaign_initialize(editor_state_t* state)
{
	editor_state = state;
	info_mode_class_t class =
	{
		.mode = MODE_CAMPAIGN,
		.caption = "Campaign",
		.initialize = campaign_info_initialize,
		.destroy = campaign_info_destroy,
		.show = campaign_info_show,
		.proc = campaign_info_proc,
		.interact_tree_item = campaign_info_handle_interact_tree_item
	};
	info_add_class(&class);
}

void campaign_destroy(void)
{
	file_campaign_unload(current_campaign);
}

static void campaign_handle_file_change(const char* directory)
{
	campaign_try_load(directory);
}

static void campaign_move_window(int addend)
{
	static int prev_mid = 0;
	y_pos -= addend;
	y_pos = min(y_pos, TARGET_HEIGHT * 13 - 1);
	y_pos = max(y_pos, 0);
	//weather_set_scroll(y_pos);
	int mid = (y_pos + TARGET_HEIGHT / 2) / TARGET_HEIGHT;
	if (prev_mid != mid)
	{
		screen_change_dirt_color(screen_sprite_dirt_color(cache[mid]));
		//weather_start(save->layer_headers[mid].weather_type, save->layer_headers[mid].weather_particle_rate, save->layer_headers[mid].weather_speed);
	}
	prev_mid = mid;
	screen_repaint();
}

static void campaign_handle_repaint(void)
{
	int top = y_pos / TARGET_HEIGHT;
	int bottom = y_pos / TARGET_HEIGHT + 1;
	screen_sprite_render(0, -y_pos % TARGET_HEIGHT, cache[top]);
	screen_sprite_render(0, TARGET_HEIGHT - y_pos % TARGET_HEIGHT, cache[bottom]);
}

static void campaign_handle_keyboard(virtual_key_t key, keyboard_control_t ctr)
{
	if (ctr == CTRL_LEFT_PRESSED && key == 'S')
	{
		if (*editor_state->current_campaign_directory || campaign_info_find_file(editor_state->current_campaign_directory, sizeof editor_state->current_campaign_directory))
		{
			file_campaign_save(editor_state->current_campaign_directory, current_campaign);
		}
	}
}

static void campaign_handle_mouse_wheel(int delta)
{
	campaign_move_window(delta * 10);
}

void campaign_start(void)
{
	info_events_t info_events =
	{
		.file_handler = campaign_handle_file_change
	};
	info_set_event_handlers(&info_events);
	screen_events_t screen_events =
	{
		.repaint = campaign_handle_repaint,
		.keyboard = campaign_handle_keyboard,
		.mouse_wheel = campaign_handle_mouse_wheel,
	};
	screen_set_event_handlers(&screen_events);

	campaign_try_load(editor_state->current_campaign_directory);
}

void campaign_end(void)
{

}