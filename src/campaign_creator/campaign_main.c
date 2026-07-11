/*
	campaign_main.c ~ RL
*/

#include "campaign_main.h"
#include "campaign_info.h"
#include "../action_buffer.h"
#include "../info_box.h"
#include "../path.h"
#include "../tool.h"
#include "../screen.h"
#include "../asset_creator/asset_util.h"
#include <stdio.h>

static void campaign_move_window(int addend);

static editor_state_t* editor_state;
static campaign_t* current_campaign;
static int y_pos;
static asset_t master_asset;
static asset_t layers[14];

static bool render_end_box;
static bool dragging_end_box;
static region_t temp_end_box;

static tool_select_t tool_select;
static action_buffer_t action_buffer;

static asset_suite_t asset_suite;

static inline asset_t* campaign_get_current_asset(void)
{
	return &layers[(y_pos + TARGET_HEIGHT / 2) / TARGET_HEIGHT];
}

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

	file_campaign_load_layers(current_campaign, &master_asset, layers);
	asset_suite.asset = &master_asset;

	asset_t* current = campaign_get_current_asset();
	screen_change_dirt_color(current->dirt_color);
	campaign_set_current_layer(current);
	return result;
}

static void campaign_asset_brush(tool_brush_t brush, region_t region)
{
	if (brush == asset_suite.tool_brush)
	{
		/*asset_block_t block;
		asset_info_get_current_brush_block(&block);
		asset_handle_brush(&asset_suite, region, block);*/
	}
	else if (brush == asset_suite.tool_eraser)
	{

	}
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

	tool_select = tool_select_create(WORLD_WIDTH, WORLD_HEIGHT);
	action_buffer = action_buffer_initialize();

	asset_suite.tool_brush = tool_brush_create(campaign_asset_brush, BRUSH_TYPE_ASSET_BLOCK, WORLD_WIDTH, WORLD_HEIGHT);
	asset_suite.tool_eraser = tool_brush_create(campaign_asset_brush, BRUSH_TYPE_ASSET_BLOCK, WORLD_WIDTH, WORLD_HEIGHT);
	asset_suite.tool_select = tool_select;
	asset_suite.buffer = action_buffer;

	campaign_info_action_buffer_set(action_buffer);
}

void campaign_destroy(void)
{
	tool_brush_destroy(asset_suite.tool_brush);
	tool_brush_destroy(asset_suite.tool_eraser);
	tool_select_destroy(asset_suite.tool_select);

	file_campaign_unload(current_campaign);
	file_campaign_unload_layers(&master_asset, layers);
	action_buffer_destroy(action_buffer);
}

static void campaign_handle_file_change(const char* directory)
{
	if (CAMPAIGN_IS_LAYER_FILE_CHANGE(directory))
	{
		file_campaign_unload_layers(&master_asset, layers);
		file_campaign_load_layers(current_campaign, &master_asset, layers);
		asset_suite.asset = &master_asset;
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

static void campaign_handle_tool_change(const info_tool_t* tool)
{
	dragging_end_box = false;
}

static void campaign_move_window(int addend)
{
	int prev = y_pos;
	y_pos -= addend;
	y_pos = min(y_pos, TARGET_HEIGHT * 13 - 1);
	y_pos = max(y_pos, 0);
	int index = (y_pos + TARGET_HEIGHT / 2) / TARGET_HEIGHT;
	if (index != (prev + TARGET_HEIGHT / 2) / TARGET_HEIGHT)
	{
		screen_change_dirt_color(layers[index].dirt_color);
		campaign_set_current_layer(&layers[index]);
	}
	screen_repaint();
}

static void campaign_handle_repaint(void)
{
	int top = y_pos / TARGET_HEIGHT;
	int bottom = y_pos / TARGET_HEIGHT + 1;
	asset_render(&layers[top], true, 0, -y_pos % TARGET_HEIGHT);
	asset_render(&layers[bottom], true, 0, TARGET_HEIGHT - y_pos % TARGET_HEIGHT);
	tool_select_render(tool_select, 0, y_pos);
	if (render_end_box)
	{
		region_t normalized = dragging_end_box ? region_validate(temp_end_box) : current_campaign->end_box;
		normalized.y0 -= y_pos;
		normalized.y1 -= y_pos;
		screen_invert_region(normalized);
	}
}

static void campaign_do_action(action_t* act)
{
	if (!act)
	{
		return;
	}
	if (act->type == ACTION_FIELD)
	{
		action_buffer_reverse_field(act);
		//asset_handle_global_field_change(serialize_element_get_value(act->sub.field.element));
		return;
	}
	else if (act->type == ACTION_ASSET_BLOCK)
	{
		action_buffer_reverse_asset_block(&master_asset, act);
	}
	else if (act->type == ACTION_BLOCK)
	{
		//action_buffer_reverse_block(, act);
	}
	screen_repaint();
	//asset_info_set_current(tool_select_region(suite.tool_select));
}

static void campaign_handle_keyboard(virtual_key_t key, keyboard_control_t ctr)
{
	if (key == VK_UP || key == VK_DOWN)
	{
		y_pos += key - VK_DOWN + 1;
	}
	if (ctr != CTRL_LEFT_PRESSED)
	{
		return;
	}
	switch (key)
	{
	case 'S':
		if (*editor_state->current_campaign_directory || campaign_info_find_file(editor_state->current_campaign_directory, sizeof editor_state->current_campaign_directory))
		{
			file_campaign_save(editor_state->current_campaign_directory, current_campaign);
		}
		break;
	case 'Z':
		campaign_do_action(action_buffer_back(action_buffer));
		break;
	case 'Y':
		campaign_do_action(action_buffer_forward(action_buffer));
		break;
	}
}

static void campaign_handle_mouse_button(bool m1_down, int x, int y)
{
	info_tool_t tool = campaign_info_get_tool();
	campaign_mode_t mode = campaign_mode();
	if (tool == TOOL_ENDBOX)
	{
		dragging_end_box = m1_down;
		if (dragging_end_box)
		{
			temp_end_box.x0 = x;
			temp_end_box.y0 = y + y_pos;
			temp_end_box.x1 = x;
			temp_end_box.y1 = y + y_pos;
		}
		else
		{
			current_campaign->end_box = region_validate(temp_end_box);
		}
		screen_repaint();
	}
	else if (tool == TOOL_SELECT)
	{
		asset_suite.scroll_y = -y_pos;
		tool_event_t event = asset_select_handle_mouse_button(&asset_suite, m1_down, x, y);
		if (event == EVENT_SELECTION_MOVE_STOP)
		{
			screen_repaint();
			//asset_info_set_current(INVALID_REGION);
		}
		else if (event == EVENT_SELECTION_RESIZE_STOP)
		{
			//asset_info_set_current(tool_select_region(suite.tool_select));
		}
		return;
	}
}

static void campaign_handle_mouse_move(bool m1_down, int x, int y)
{
	info_tool_t tool = campaign_info_get_tool();
	if (tool == TOOL_ENDBOX)
	{
		if (!m1_down)
		{
			return;
		}
		int new_selected_y = y + y_pos;
		temp_end_box.x1 = min(x, temp_end_box.x0 + 80 - 1);
		temp_end_box.y1 = min(new_selected_y, temp_end_box.y0 + 80 - 1);
		temp_end_box.x1 = max(temp_end_box.x1, temp_end_box.x0 - 80 + 1);
		temp_end_box.y1 = max(temp_end_box.y1, temp_end_box.y0 - 80 + 1);
		temp_end_box = region_keep_inside((region_t){ 0, 0, WORLD_WIDTH, WORLD_HEIGHT }, temp_end_box);
		screen_repaint();
	}
	else if (tool == TOOL_SELECT)
	{
		tool_select_handle_mouse_move(tool_select, m1_down, x, y, 0, y_pos);
		screen_repaint();
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
		.tool_handler = campaign_handle_tool_change,
	};
	info_set_event_handlers(&info_events);
	screen_events_t screen_events =
	{
		.repaint = campaign_handle_repaint,
		.keyboard = campaign_handle_keyboard,
		.mouse_button = campaign_handle_mouse_button,
		.mouse_move = campaign_handle_mouse_move,
		.mouse_wheel = campaign_handle_mouse_wheel,
	};
	screen_set_event_handlers(&screen_events);

	campaign_try_load(editor_state->current_campaign_directory);
	screen_change_dirt_color(campaign_get_current_asset()->dirt_color);
	screen_repaint();
}

void campaign_end(void)
{

}

bool campaign_can_change_field(const void* field)
{
	return field != &campaign_get_current_asset()->width && field != &campaign_get_current_asset()->height;
}