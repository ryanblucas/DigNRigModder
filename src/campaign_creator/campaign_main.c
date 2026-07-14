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
#include "../weather.h"
#include "../asset_creator/asset_util.h"
#include <stdio.h>

static void campaign_move_window(int addend);

static editor_state_t* editor_state;
static campaign_t* current_campaign;
static int y_pos;
static asset_t master_asset;
static asset_t layers[LAYER_COUNT];
static uint64_t layer_hashes[LAYER_COUNT];
static bool dont_ask_save_asset;

static int flag_pos[2];

static bool render_start_and_end;
static bool moving_flag;
static bool dragging_end_box;
static region_t temp_end_box;

static tool_select_t tool_select;
static action_buffer_t action_buffer;

static asset_suite_t asset_suite;

static sprite_t flag;

static inline asset_t* campaign_get_current_asset(void)
{
	return &layers[(y_pos + TARGET_HEIGHT / 2) / TARGET_HEIGHT];
}

static void campaign_hash_layers(uint64_t* hashes)
{
	for (int i = 0; i < LAYER_COUNT; i++)
	{
		hashes[i] = dig_hash_buf(layers[i].blocks, TARGET_WIDTH * TARGET_HEIGHT * sizeof * layers[i].blocks);
		hashes[i] = dig_hash_update_buf(hashes[i], &layers[i], sizeof layers[i]);
	}
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

	file_campaign_load_layers(current_campaign, &master_asset, layers);
	asset_suite.asset = &master_asset;
	campaign_info_set(current_campaign, &master_asset, editor_state->current_campaign_directory);
	campaign_hash_layers(layer_hashes);

	asset_t* current = campaign_get_current_asset();
	screen_change_dirt_color(current->dirt_color);
	campaign_set_current_layer(current);

	weather_force_end();
	weather_set_asset(&master_asset);
	weather_start(current->weather_type, current->weather_particle_rate, current->weather_speed);

	return result;
}

static void campaign_asset_brush(tool_brush_t brush, region_t region)
{
	if (brush == asset_suite.tool_brush)
	{
		asset_block_t block;
		campaign_info_get_current_brush_block(&block);
		asset_handle_brush(&asset_suite, region, block);
	}
	else if (brush == asset_suite.tool_eraser)
	{
		asset_handle_erase(&asset_suite, region);
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

	char buf[MAX_PATH];
	asset_t asset = file_asset_load(path_find_dnr_main(buf, sizeof buf, "Sprites\\Checkpoint.sprite"));
	flag = game_spritify_asset(asset);
	file_asset_unload(&asset);
	RUNTIME_ASSERT(flag);
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
		render_start_and_end = id == CPI_ENABLE_END_BOX;
	}
	screen_repaint();
}

static void campaign_handle_tool_change(const info_tool_t* tool)
{
	dragging_end_box = false;
	tool_select_reset(tool_select);
	screen_repaint();
}

static void campaign_handle_global_field_change(const void* field)
{
	asset_t* curr = campaign_get_current_asset();
	if (field == &curr->weather_type || field == &curr->weather_particle_rate || field == &curr->weather_speed)
	{
		weather_force_end();
		weather_start(curr->weather_type, curr->weather_particle_rate, curr->weather_speed);
	}
	else if (field == &curr->dirt_color)
	{
		screen_change_dirt_color(curr->dirt_color);
		screen_repaint();
	}
	int index = (int)(((uintptr_t)curr - (uintptr_t)layers) / sizeof(asset_t));
	for (int i = 0; i < LAYER_COUNT; i++)
	{
		if (index != i && strncmp(current_campaign->layers[i].directory, current_campaign->layers[index].directory, MAX_PATH) == 0)
		{
			asset_block_t* original = layers[i].blocks;
			layers[i] = layers[index];
			layers[i].blocks = original;
		}
	}
}

static void campaign_handle_block_change(const region_t* region)
{
	int index = region->y0 / TARGET_HEIGHT;
	int layer2 = region->y1 / TARGET_HEIGHT;
	asset_block_t* blocks = dig_malloc(sizeof * blocks * TARGET_WIDTH * TARGET_HEIGHT);
	while (index <= layer2)
	{
		game_asset_copy(&master_asset, (region_t) { 0, index * TARGET_HEIGHT, TARGET_WIDTH - 1, (index + 1) * TARGET_HEIGHT - 1 }, blocks);
		for (int i = 0; i < LAYER_COUNT; i++)
		{
			if (i != index && strncmp(current_campaign->layers[i].directory, current_campaign->layers[index].directory, MAX_PATH) == 0)
			{
				game_asset_paste(&master_asset, (region_t) { 0, i * TARGET_HEIGHT, TARGET_WIDTH - 1, (i + 1) * TARGET_HEIGHT - 1 }, blocks);
			}
		}
		index++;
	}
	free(blocks);
	screen_repaint();
}

static void campaign_handle_brush_size_change(const int* new_size)
{
	tool_brush_set_size(asset_suite.tool_brush, *new_size);
	tool_brush_set_size(asset_suite.tool_eraser, *new_size);
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
		weather_start(layers[index].weather_type, layers[index].weather_particle_rate, layers[index].weather_speed);
	}
	asset_suite.scroll_y = -y_pos;
	weather_set_scroll(y_pos);
	screen_repaint();
}

static void campaign_handle_repaint(void)
{
	int top = y_pos / TARGET_HEIGHT;
	int bottom = y_pos / TARGET_HEIGHT + 1;
	asset_render(&layers[top], true, 0, -y_pos % TARGET_HEIGHT);
	asset_render(&layers[bottom], true, 0, TARGET_HEIGHT - y_pos % TARGET_HEIGHT);
	if (render_start_and_end)
	{
		if (current_campaign->start_y > top * TARGET_HEIGHT && current_campaign->start_y < (bottom + 1) * TARGET_HEIGHT)
		{
			screen_sprite_render(current_campaign->start_x, current_campaign->start_y - y_pos, flag);
		}
		region_t normalized = dragging_end_box ? region_validate(temp_end_box) : current_campaign->end_box;
		normalized.y0 -= y_pos;
		normalized.y1 -= y_pos;
		screen_invert_region(normalized);
	}
	tool_select_render(tool_select, 0, y_pos);
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
		campaign_handle_global_field_change(serialize_element_get_value(act->sub.field.element));
	}
	else if (act->type == ACTION_ASSET_BLOCK)
	{
		action_buffer_reverse_asset_block(&master_asset, act);
		campaign_handle_block_change(&act->sub.asset.region);
	}
	else if (act->type == ACTION_BLOCK)
	{
		//action_buffer_reverse_block(, act);
	}
	else if (act->type == ACTION_MEMORY)
	{
		action_buffer_reverse_memory(act);
		if (act->sub.memory.type == 0)
		{
			current_campaign->start_x = flag_pos[0];
			current_campaign->start_y = flag_pos[1];
		}
	}
	screen_repaint();
	campaign_set_current_region(tool_select_region(tool_select));
}

static void campaign_handle_save(void)
{
	debug_format("Saving to disk...\n");
	if (*editor_state->current_campaign_directory || campaign_info_find_file(editor_state->current_campaign_directory, sizeof editor_state->current_campaign_directory))
	{
		file_campaign_save(editor_state->current_campaign_directory, current_campaign);
	}
	uint64_t new_hashes[LAYER_COUNT];
	campaign_hash_layers(new_hashes);
	for (int i = 0; i < LAYER_COUNT; i++)
	{
		if (new_hashes[i] == layer_hashes[i])
		{
			continue;
		}
		layer_hashes[i] = new_hashes[i];
		if (!dont_ask_save_asset)
		{
			if (MessageBoxA(NULL, "Save this and future layer changes to their respective source files during this session?\nIf you say yes, you can see this menu again by doing CTRL-SHIFT-S instead.", "Save Changes", MB_YESNO | MB_ICONQUESTION) == IDNO)
			{
				continue;
			}
			dont_ask_save_asset = true;
		}
		debug_format("Saving layer (%s, %i) to disk...\n", current_campaign->layers[i].directory, i);
		file_asset_save(current_campaign->layers[i].directory, &layers[i]);
	}
}

static void campaign_handle_keyboard(virtual_key_t key, keyboard_control_t ctr)
{
	if (key == VK_UP || key == VK_DOWN)
	{
		y_pos += key - VK_DOWN + 1;
	}
	if (!(ctr & CTRL_LEFT_PRESSED))
	{
		return;
	}
	switch (key)
	{
	case 'S':
		if (ctr & CTRL_SHIFT_PRESSED)
		{
			dont_ask_save_asset = false;
		}
		campaign_handle_save();
		break;
	case 'Z':
		campaign_do_action(action_buffer_back(action_buffer));
		break;
	case 'Y':
		campaign_do_action(action_buffer_forward(action_buffer));
		break;
	}
}

static void campaign_end_box_handle_mouse_button(bool m1_down, int x, int y)
{
	if (!m1_down)
	{
		if (moving_flag)
		{
			int prev[2] = { flag_pos[0], flag_pos[1] };
			flag_pos[0] = current_campaign->start_x;
			flag_pos[1] = current_campaign->start_y;
			action_buffer_add_memory(action_buffer, 0, sizeof flag_pos, flag_pos, prev);
			moving_flag = false;
			return;
		}
		region_t prev = current_campaign->end_box;
		current_campaign->end_box = region_validate(temp_end_box);
		action_buffer_add_memory(action_buffer, 1, sizeof temp_end_box, &current_campaign->end_box, &prev);
		dragging_end_box = false;
		return;
	}
	region_t flag_region = { current_campaign->start_x, current_campaign->start_y, current_campaign->start_x + screen_sprite_width(flag), current_campaign->start_y + screen_sprite_height(flag) };
	if (region_is_inside(flag_region, x, y + y_pos))
	{
		moving_flag = true;
		flag_pos[0] = current_campaign->start_x;
		flag_pos[1] = current_campaign->start_y;
		return;
	}
	dragging_end_box = true;
	temp_end_box.x0 = x;
	temp_end_box.y0 = y + y_pos;
	temp_end_box.x1 = x;
	temp_end_box.y1 = y + y_pos;
	screen_repaint();
}

static void campaign_handle_mouse_button(bool m1_down, int x, int y)
{
	info_tool_t tool = campaign_info_get_tool();
	campaign_mode_t mode = campaign_mode();
	if (tool == TOOL_ENDBOX)
	{
		campaign_end_box_handle_mouse_button(m1_down, x, y);
	}
	else if (tool == TOOL_SELECT)
	{
		tool_event_t event = asset_select_handle_mouse_button(&asset_suite, m1_down, x, y);
		if (event == EVENT_SELECTION_MOVE_STOP)
		{
			region_t temp = region_merge(tool_select_move_region(tool_select), tool_select_region(tool_select));
			campaign_handle_block_change(&temp);
			screen_repaint();
			campaign_set_current_region(INVALID_REGION);
		}
		else if (event == EVENT_SELECTION_RESIZE_STOP)
		{
			campaign_set_current_region(tool_select_region(tool_select));
		}
		return;
	}
	else if (tool == TOOL_BRUSH)
	{
		if (asset_brush_handle_mouse_button(&asset_suite, false, m1_down, x, y) == EVENT_BRUSH_END)
		{
			region_t temp = tool_brush_region(asset_suite.tool_brush);
			campaign_handle_block_change(&temp);
		}
	}
	else if (tool == TOOL_ERASER)
	{
		if (asset_brush_handle_mouse_button(&asset_suite, true, m1_down, x, y) == EVENT_BRUSH_END)
		{
			region_t temp = tool_brush_region(asset_suite.tool_brush);
			campaign_handle_block_change(&temp);
		}
	}
}

static void campaign_brush_handle_mouse_move(tool_brush_t brush, bool m1_down, int x, int y)
{
	tool_event_t result = tool_brush_handle_mouse_move(brush, m1_down, x, y, y_pos);
	if (!region_is_invalid(tool_brush_region(brush)))
	{
		screen_repaint();
	}
}

static void campaign_end_box_handle_mouse_move(bool m1_down, int x, int y)
{
	if (!m1_down)
	{
		return;
	}
	if (moving_flag)
	{
		current_campaign->start_x = x;
		current_campaign->start_y = y + y_pos;
		screen_repaint();
		return;
	}
	int new_selected_y = y + y_pos;
	temp_end_box.x1 = min(x, temp_end_box.x0 + 80 - 1);
	temp_end_box.y1 = min(new_selected_y, temp_end_box.y0 + 80 - 1);
	temp_end_box.x1 = max(temp_end_box.x1, temp_end_box.x0 - 80 + 1);
	temp_end_box.y1 = max(temp_end_box.y1, temp_end_box.y0 - 80 + 1);
	temp_end_box = region_keep_inside((region_t) { 0, 0, WORLD_WIDTH, WORLD_HEIGHT }, temp_end_box);
	screen_repaint();
}

static void campaign_handle_mouse_move(bool m1_down, int x, int y)
{
	info_tool_t tool = campaign_info_get_tool();
	if (tool == TOOL_ENDBOX)
	{
		campaign_end_box_handle_mouse_move(m1_down, x, y);
	}
	else if (tool == TOOL_SELECT)
	{
		tool_select_handle_mouse_move(tool_select, m1_down, x, y, 0, y_pos);
		screen_repaint();
	}
	else if (tool == TOOL_BRUSH)
	{
		campaign_brush_handle_mouse_move(asset_suite.tool_brush, m1_down, x, y);
	}
	else if (tool == TOOL_ERASER)
	{
		campaign_brush_handle_mouse_move(asset_suite.tool_eraser, m1_down, x, y);
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
		.global_field_handler = campaign_handle_global_field_change,
		.block_handler = campaign_handle_block_change,
		.brush_size_handler = campaign_handle_brush_size_change,
	};
	info_set_event_handlers(&info_events);
	screen_simulator_t simulators[] = { weather_simulate, NULL };
	screen_events_t screen_events =
	{
		.repaint = campaign_handle_repaint,
		.keyboard = campaign_handle_keyboard,
		.mouse_button = campaign_handle_mouse_button,
		.mouse_move = campaign_handle_mouse_move,
		.mouse_wheel = campaign_handle_mouse_wheel,
		.simulators = simulators
	};
	screen_set_event_handlers(&screen_events);

	campaign_try_load(editor_state->current_campaign_directory);
	screen_change_dirt_color(campaign_get_current_asset()->dirt_color);
	screen_repaint();
}

void campaign_end(void)
{
	dragging_end_box = false;
	moving_flag = false;
	weather_force_end();
	tool_select_reset(tool_select);
}

bool campaign_can_change_field(const void* field)
{
	return field != &campaign_get_current_asset()->width && field != &campaign_get_current_asset()->height;
}