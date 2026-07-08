/*
	asset_main.c ~ RL
*/

#include "asset_main.h"
#include "asset_info.h"
#include "asset_util.h"
#include "../action_buffer.h"
#include "../info_box.h"
#include "../screen.h"
#include "../tool.h"
#include "../weather.h"
#include <stdio.h>

#define ASSET_LOADED() (!!suite.asset.blocks)

static char directory[MAX_PATH];
static int old_width, old_height;
static bool is_layer;

static editor_state_t* editor_state;
static asset_suite_t suite;

static void asset_invalidate(void)
{
	screen_repaint();
}

static void asset_brush(tool_brush_t brush, region_t region)
{
	RUNTIME_ASSERT(brush == suite.tool_brush);
	asset_block_t block;
	asset_info_get_current_brush_block(&block);
	asset_handle_brush(&suite, region, block);
}

static void asset_erase(tool_brush_t brush, region_t region)
{
	RUNTIME_ASSERT(brush == suite.tool_eraser);
	asset_handle_erase(&suite, region);
}

static void asset_refresh(void)
{
	old_width = suite.asset.width;
	old_height = suite.asset.height;

	asset_info_set(&suite.asset);
	weather_set_asset(&suite.asset);

	suite.scroll_x = TARGET_WIDTH / 2 - suite.asset.width / 2;
	suite.scroll_y = TARGET_HEIGHT / 2 - suite.asset.height / 2;

	tool_brush_destroy(suite.tool_eraser);
	tool_brush_destroy(suite.tool_brush);
	tool_select_destroy(suite.tool_select);

	suite.tool_brush = tool_brush_create(asset_brush, BRUSH_TYPE_ASSET_BLOCK, suite.asset.width, suite.asset.height);
	suite.tool_eraser = tool_brush_create(asset_erase, BRUSH_TYPE_ASSET_BLOCK, suite.asset.width, suite.asset.height);
	suite.tool_select = tool_select_create(suite.asset.width, suite.asset.height);
}

static void asset_handle_file_change(const char* _directory)
{
	screen_clear();

	file_asset_unload(&suite.asset);

	snprintf(directory, sizeof directory, "%s", _directory);
	snprintf(editor_state->current_asset_directory, sizeof editor_state->current_asset_directory, "%s", _directory);
	suite.asset = file_asset_load(directory);
	RUNTIME_ASSERT(suite.asset.blocks);

	is_layer = false;
	char* end = strrchr(directory, '.');
	if (end && strncmp(end, ".layer", 6) == 0)
	{
		is_layer = true;
	}

	asset_refresh();
	asset_invalidate();

	weather_force_end();
	weather_start(suite.asset.weather_type, suite.asset.weather_particle_rate, suite.asset.weather_speed);
}

static void asset_handle_block_change(const region_t* region)
{
	asset_invalidate();
}

static void asset_handle_tool_change(const info_tool_t* new_tool)
{
	tool_select_reset(suite.tool_select);
	if (ASSET_LOADED())
	{
		screen_repaint();
	}
}

static void asset_handle_brush_size_change(const int* new_size)
{
	tool_brush_set_size(suite.tool_brush, *new_size);
	tool_brush_set_size(suite.tool_eraser, *new_size);
}

static bool asset_change_dimensions(asset_t* copy, int* field, int max)
{
	if (is_layer)
	{
		*field = max;
		return false;
	}
	*field = min(max(1, *field), max);
	*copy = suite.asset;
	copy->blocks = dig_malloc(sizeof * copy->blocks * suite.asset.width * suite.asset.height);
	memset(copy->blocks, 0, sizeof * copy->blocks * suite.asset.width * suite.asset.height);
	return true;
}

static void asset_handle_global_field_change(const void* field)
{
	if (field == &suite.asset.weather_type || field == &suite.asset.weather_particle_rate || field == &suite.asset.weather_speed)
	{
		weather_force_end();
		weather_start(suite.asset.weather_type, suite.asset.weather_particle_rate, suite.asset.weather_speed);
	}
	asset_t copy;
	if (field == &suite.asset.width && asset_change_dimensions(&copy, &suite.asset.width, TARGET_WIDTH))
	{
		for (int i = 0; i < suite.asset.height; i++)
		{
			memcpy(copy.blocks + i * suite.asset.width, suite.asset.blocks + i * old_width, sizeof * copy.blocks * min(old_width, suite.asset.width));
		}
		file_asset_unload(&suite.asset);
		suite.asset = copy;
		asset_refresh();
	}
	if (field == &suite.asset.height && asset_change_dimensions(&copy, &suite.asset.height, TARGET_HEIGHT))
	{
		memcpy(copy.blocks, suite.asset.blocks, sizeof * copy.blocks * suite.asset.width * min(old_height, suite.asset.height));
		file_asset_unload(&suite.asset);
		suite.asset = copy;
		asset_refresh();
	}
	asset_invalidate();
}

static void asset_handle_repaint(void)
{
	if (!ASSET_LOADED())
	{
		return;
	}
	screen_change_dirt_color(suite.asset.dirt_color);
	asset_render(&suite.asset, is_layer, suite.scroll_x, suite.scroll_y);

	tool_select_render(suite.tool_select, -suite.scroll_x, -suite.scroll_y);
	char ch = ' ';
	attribute_t attrib = 0;
	for (int y = 0; y < TARGET_HEIGHT; y++)
	{
		for (int x = 0; x < TARGET_WIDTH; x++)
		{
			if (x < suite.scroll_x || x >= suite.scroll_x + suite.asset.width || y < suite.scroll_y || y >= suite.scroll_y + suite.asset.height)
			{
				screen_set_attrib_region(&attrib, (region_t) { x, y, x, y });
				screen_set_char_region(&ch, (region_t) { x, y, x, y });
			}
		}
	}
}

static void asset_handle_mouse_button(bool m1_down, int x, int y)
{
	info_tool_t current = asset_info_get_current_tool();
	if (current == TOOL_SELECT)
	{
		tool_event_t event = asset_select_handle_mouse_button(&suite, m1_down, x, y);
		if (event == EVENT_SELECTION_MOVE_STOP)
		{
			asset_invalidate();
			asset_info_set_current(INVALID_REGION);
		}
		else if (event == EVENT_SELECTION_RESIZE_STOP)
		{
			asset_info_set_current(tool_select_region(suite.tool_select));
		}
		return;
	}
	x -= suite.scroll_x;
	y -= suite.scroll_y;
	if (x >= suite.asset.width || y >= suite.asset.height || x < 0 || y < 0)
	{
		return;
	}
	if (current == TOOL_BRUSH)
	{
		asset_brush_handle_mouse_button(&suite, false, m1_down, x, y);
	}
	else if (current == TOOL_ERASER)
	{
		asset_brush_handle_mouse_button(&suite, true, m1_down, x, y);
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

static void asset_handle_mouse_move(bool m1_down, int x, int y)
{
	info_tool_t current = asset_info_get_current_tool();
	if (current == TOOL_SELECT)
	{
		tool_select_handle_mouse_move(suite.tool_select, m1_down, x, y, -suite.scroll_x, -suite.scroll_y);
		asset_invalidate();
		return;
	}
	x -= suite.scroll_x;
	y -= suite.scroll_y;
	if (x >= suite.asset.width || y >= suite.asset.height || x < 0 || y < 0)
	{
		return;
	}
	if (current == TOOL_BRUSH)
	{
		asset_brush_handle_mouse_move(suite.tool_brush, m1_down, x, y);
	}
	else if (current == TOOL_ERASER)
	{
		asset_brush_handle_mouse_move(suite.tool_eraser, m1_down, x, y);
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
		action_buffer_reverse_field(act);
		asset_handle_global_field_change(serialize_element_get_value(act->sub.field.element));
		return;
	}
	action_buffer_reverse_asset_block(&suite.asset, act);
	asset_invalidate();
	asset_info_set_current(tool_select_region(suite.tool_select));
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
		asset_do_action(action_buffer_back(suite.buffer));
		break;
	case 'Y':
		asset_do_action(action_buffer_forward(suite.buffer));
		break;
	case 'S':
		debug_format("Saving to disk...\n");
		if (!file_asset_save(directory, &suite.asset))
		{
			MessageBoxW(NULL, L"Failed to save file, maybe run in admin mode?", L"Dig-N-Rig Modder - Error!", MB_OK | MB_ICONERROR);
		}
		break;
	case 'C':
		asset_handle_copy(&suite);
		break;
	case 'V':
		asset_handle_paste(&suite);
		asset_invalidate();
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

	suite.tool_eraser = tool_brush_create(asset_erase, BRUSH_TYPE_ASSET_BLOCK, TARGET_WIDTH, TARGET_HEIGHT);
	suite.tool_brush = tool_brush_create(asset_brush, BRUSH_TYPE_ASSET_BLOCK, TARGET_WIDTH, TARGET_HEIGHT);
	suite.tool_select = tool_select_create(TARGET_WIDTH, TARGET_HEIGHT);
	suite.buffer = action_buffer_initialize();

	asset_info_action_buffer_set(suite.buffer);
}

void asset_destroy(void)
{
	file_asset_unload(&suite.asset);
	tool_select_destroy(suite.tool_select);
	action_buffer_destroy(suite.buffer);
	free(suite.clipboard_data);
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
	tool_select_reset(suite.tool_select);
	asset_info_palette_save(editor_state->asset_palette, sizeof editor_state->asset_palette / sizeof * editor_state->asset_palette);
}

bool asset_can_change_field(const void* field)
{
	return !is_layer || (field != &suite.asset.width && field != &suite.asset.height);
}