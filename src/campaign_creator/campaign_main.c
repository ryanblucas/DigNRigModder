/*
	campaign_main.c ~ RL
*/

#include "campaign_main.h"
#include "campaign_info.h"
#include "../info_box.h"
#include "../path.h"
#include "../screen.h"
#include "../asset_creator/asset_util.h"
#include <stdio.h>

static editor_state_t* editor_state;
static campaign_t* current_campaign;
static int y_pos;
static asset_t layers[14];

static bool render_end_box;

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
		char buf[MAX_PATH];
		path_find_dnr_main_chain(buf, sizeof buf, "Layers", current_campaign->layers[i].directory);
		layers[i] = file_asset_load(buf);
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
	if (CAMPAIGN_IS_LAYER_FILE_CHANGE(directory))
	{
		char buf[MAX_PATH];
		int index = CAMPAIGN_CONVERT_FILE_CHANGE_PARAM(directory);
		asset_t* asset = &layers[index];
		file_asset_unload(asset);
		path_find_dnr_main_chain(buf, sizeof buf, "Layers", current_campaign->layers[index].directory);
		*asset = file_asset_load(buf);
		screen_repaint();
		return;
	}
	campaign_try_load(directory);
}

static void campaign_handle_custom_event(const int* _id)
{
	campaign_property_id_t id = (campaign_property_id_t)_id;
	if (id == CPI_ENABLE_END_BOX || id == CPI_DISABLE_END_BOX)
	{
		render_end_box = id == CPI_ENABLE_END_BOX;
	}
	screen_repaint();
}

static void campaign_move_window(int addend)
{
	y_pos -= addend;
	y_pos = min(y_pos, TARGET_HEIGHT * 13 - 1);
	y_pos = max(y_pos, 0);
	screen_change_dirt_color(layers[(y_pos + TARGET_HEIGHT / 2) / TARGET_HEIGHT].dirt_color);
	screen_repaint();
}

static void campaign_handle_repaint(void)
{
	int top = y_pos / TARGET_HEIGHT;
	int bottom = y_pos / TARGET_HEIGHT + 1;
	asset_render(&layers[top], true, 0, -y_pos % TARGET_HEIGHT);
	asset_render(&layers[bottom], true, 0, TARGET_HEIGHT - y_pos % TARGET_HEIGHT);
	if (render_end_box)
	{
		region_t normalized = current_campaign->end_box;
		normalized.y0 -= y_pos;
		normalized.y1 -= y_pos;
		screen_invert_region(normalized);
	}
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
		.file_handler = campaign_handle_file_change,
		.custom_event_handler = campaign_handle_custom_event,
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
	screen_change_dirt_color(layers[(y_pos + TARGET_HEIGHT / 2) / TARGET_HEIGHT].dirt_color);
}

void campaign_end(void)
{

}