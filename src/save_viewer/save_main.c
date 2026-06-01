/*
	save_main.c ~ RL
	Views save files
*/

#include "save_info.h"
#include "../action_buffer.h"
#include "../interface/change_field_modal.h"
#include "../file.h"
#include "../game.h"
#include "../info_box.h"
#include "../path.h"
#include "../screen.h"
#include "../tool.h"
#include <stdio.h>

#define MAX_SELECTION_WIDTH 120
#define MAX_SELECTION_HEIGHT 80
#define MAX_SELECTION_SIZE (MAX_SELECTION_WIDTH * MAX_SELECTION_HEIGHT)

static void save_viewer_handle_global_field_change(const void* field);

static action_buffer_t action_buffer;

static sprite_t flag;
static sprite_t cache[LAYER_COUNT];
static dnr_state_t* save;
static int y_pos;

static tool_brush_t eraser_tool;
static tool_brush_t brush_tool;
static tool_select_t select_tool;

static char save_directory[MAX_PATH];
static editor_state_t* editor;

static region_t clipboard_region;
static complete_block_t* clipboard_data;

static inline void save_viewer_invalidate_region(region_t region)
{
	RUNTIME_ASSERT(!region_is_invalid(region));
	region = region_validate(region);
	for (int y = region.y0 / TARGET_HEIGHT; y <= region.y1 / TARGET_HEIGHT; y++)
	{
		screen_sprite_destroy(cache[y]);
		cache[y] = game_spritify_layer(save, y);
	}
}

static void save_viewer_copy(void)
{
	if (region_is_invalid(tool_select_region(select_tool)))
	{
		return;
	}

	if (clipboard_data)
	{
		free(clipboard_data);
	}

	clipboard_region = tool_select_region(select_tool);
	clipboard_data = dig_malloc(region_size(clipboard_region) * sizeof * clipboard_data);
	game_copy(save, clipboard_region, clipboard_data);
}

static void save_viewer_paste(void)
{
	region_t region = tool_select_region(select_tool);
	if (!clipboard_data || region_is_invalid(region))
	{
		return;
	}

	region_t dest = { region.x0, region.y0, region.x0 + region_width(clipboard_region) - 1, region.y0 + region_height(clipboard_region) - 1 };

	if (!dig_inside_bounds(dest.x0, dest.y0) || !dig_inside_bounds(dest.x1, dest.y1))
	{
		return;
	}

	action_buffer_pre_add_block(action_buffer, save, dest);

	game_delete(save, region);
	game_paste(save, dest, clipboard_data);

	action_buffer_post_add_block(action_buffer, save);

	tool_select_set_region(select_tool, dest);
	save_viewer_invalidate_region(dest);
	screen_repaint();
}

static void save_viewer_prompt_which_save(void)
{
	bool valid_save = false;
	while (editor->current_save <= 0 || editor->current_save >= 4 || !valid_save)
	{
		/* temporary use of change field modal function */
		change_field_modal_integer(NULL, &editor->current_save, sizeof editor->current_save | SIZE_IS_SIGNED);
		char directory[MAX_PATH];
		path_find_dnr_save(directory, sizeof directory, editor->current_save);
		FILE* file = fopen(directory, "rb");
		if (!file)
		{
			continue;
		}
		fseek(file, 0, SEEK_END);
		long file_size = ftell(file);
		fclose(file);
		if (file_size <= 0)
		{
			continue;
		}
		valid_save = true;
	}
}

static void save_viewer_move_window(int addend)
{
	static int prev_mid = 0;
	y_pos -= addend;
	y_pos = min(y_pos, TARGET_HEIGHT * 13 - 1);
	y_pos = max(y_pos, 0);
	int mid = (y_pos + TARGET_HEIGHT / 2) / TARGET_HEIGHT;
	if (prev_mid != mid)
	{
		screen_change_dirt_color(screen_sprite_dirt_color(cache[mid]));
	}
	prev_mid = mid;
	screen_repaint();
}

static void save_viewer_handle_repaint()
{
	int top = y_pos / TARGET_HEIGHT;
	int bottom = y_pos / TARGET_HEIGHT + 1;
	screen_sprite_render(0, -y_pos % TARGET_HEIGHT, cache[top]);
	screen_sprite_render(0, TARGET_HEIGHT - y_pos % TARGET_HEIGHT, cache[bottom]);

	if (save->player.y_spawn > top * TARGET_HEIGHT && save->player.y_spawn < (bottom + 1) * TARGET_HEIGHT)
	{
		screen_sprite_render((int)save->player.x_spawn, (int)save->player.y_spawn - y_pos, flag);
	}

	attribute_t attrib = CREATE_ATTRIBUTE(LIGHT_RED, DARK_BLACK);
	char ch = 'X';
	for (int y = 0; y < TARGET_HEIGHT; y++)
	{
		for (int x = 0; x < TARGET_WIDTH; x++)
		{
			dnr_block_t* block = &save->blocks[x * WORLD_HEIGHT + y + y_pos];
			if (block->enemy_exists)
			{
				screen_set_attrib_region(&attrib, (region_t) { x, y, x, y });
				screen_set_char_region(&ch, (region_t) { x, y, x, y });
			}
		}
	}

	tool_select_render(select_tool, 0, y_pos);
}

static void save_viewer_delete_selection(void)
{
	region_t region = tool_select_region(select_tool);
	if (region_is_invalid(region))
	{
		return;
	}
	debug_profiler_push();
	action_buffer_pre_add_block(action_buffer, save, region);

	game_delete(save, region);

	action_buffer_post_add_block(action_buffer, save);
	save_viewer_invalidate_region(region);
	tool_select_reset(select_tool);
	screen_repaint();
	debug_profiler_pop("Deleting region");
}

static void save_viewer_do_action(action_t* act)
{
	if (!act)
	{
		return;
	}
	if (act->type == ACTION_FIELD)
	{
		action_buffer_reverse_field(act);
		save_viewer_handle_global_field_change(serialize_element_get_value(act->sub.field.element));
		return;
	}
	action_buffer_reverse_block(save, act);
	save_viewer_invalidate_region(act->sub.block.region);
	screen_repaint();
}

static void save_viewer_handle_keyboard(virtual_key_t vk, keyboard_control_t ctrl)
{
	if (~ctrl & CTRL_LEFT_PRESSED)
	{
		switch (vk)
		{
		case VK_UP:
			save_viewer_move_window(1);
			break;
		case VK_DOWN:
			save_viewer_move_window(-1);
			break;
		case VK_DELETE:
			save_viewer_delete_selection();
			break;
		}
		return;
	}

	switch (vk)
	{
	case 'S':
		debug_format("Saving to disk...\n");
		if (!file_state_save(save_directory, save))
		{
			MessageBoxW(NULL, L"Failed to save file, maybe run in admin mode?", L"Dig-N-Rig Modder - Error!", MB_OK | MB_ICONERROR);
		}
		break;
	case 'R':
	{
		if (ctrl & CTRL_SHIFT_PRESSED)
		{
			save_viewer_prompt_which_save();
			path_find_dnr_save(save_directory, sizeof save_directory, editor->current_save);
		}
		debug_format("Reloading save...\n");
		dnr_state_t* next = file_state_load(save_directory);
		if (!next)
		{
			debug_format("Failed to reopen save! Try saving again.\n");
			return;
		}
		file_state_unload(save);
		save = next;

		save_viewer_invalidate_region((region_t) { 0, 0, WORLD_WIDTH - 1, WORLD_HEIGHT - 1 });

		save_info_state_set(save);
		screen_repaint();
		break;
	}
	case 'Z':
		save_viewer_do_action(action_buffer_back(action_buffer));
		break;
	case 'Y':
		save_viewer_do_action(action_buffer_forward(action_buffer));
		break;
	case 'C':
		save_viewer_copy();
		break;
	case 'V':
		save_viewer_paste();
		break;
	}
}

static void save_viewer_select_handle_mouse_button(bool m1_down, int x, int y)
{
	tool_event_t result = tool_select_handle_mouse_click(select_tool, m1_down, x, y, y_pos);
	switch (result)
	{
	case EVENT_SELECTION_MOVE_STOP:
	{
		region_t dest_region = tool_select_move_region(select_tool);
		region_t src_region = tool_select_region(select_tool);
		region_t total = region_keep_inside(region_merge(src_region, dest_region), (region_t){ .x1 = WORLD_WIDTH - 1, .y1 = WORLD_HEIGHT - 1 });
		action_buffer_pre_add_block(action_buffer, save, total);

		src_region.x1 = src_region.x0 + region_width(dest_region) - 1;
		src_region.y1 = src_region.y0 + region_height(dest_region) - 1;
		complete_block_t* blocks = dig_malloc(region_size(src_region) * sizeof * blocks);
		game_copy(save, src_region, blocks);
		game_delete(save, tool_select_region(select_tool));
		game_paste(save, dest_region, blocks);
		free(blocks);

		action_buffer_post_add_block(action_buffer, save);

		save_viewer_invalidate_region(total);
		break;
	}
	case EVENT_SELECTION_RESIZE_STOP:
	{
		if (region_size(tool_select_region(select_tool)) > 1)
		{
			save_info_cell_set_current_region(tool_select_region(select_tool));
		}
		else
		{
			save_info_cell_set_current(x, y + y_pos);
		}
		break;
	}
	}
	screen_repaint();
}

static void save_viewer_select_handle_mouse_move(bool m1_down, int x, int y)
{
	tool_event_t result = tool_select_handle_mouse_move(select_tool, m1_down, x, y, y_pos);
	if (result == EVENT_SELECTION_RESIZE)
	{
		if (region_size(tool_select_region(select_tool)) != 1)
		{
			save_info_cell_set_current(-1, -1);
		}
	}
	screen_repaint();
}

static void save_viewer_erase(tool_brush_t brush, region_t region)
{
	RUNTIME_ASSERT(brush == eraser_tool);

	complete_block_t* temp = dig_malloc(sizeof * temp * region_size(region));
	game_copy(save, region, temp);

	game_delete(save, region);

	for (int i = 0; i < region_size(region); i++)
	{
		tool_brush_add_to_before_list_cb(brush, &temp[i]);
	}
	free(temp);
}

static void save_viewer_brush(tool_brush_t brush, region_t region)
{
	RUNTIME_ASSERT(brush == brush_tool);

	complete_block_t* temp = dig_malloc(sizeof * temp * region_size(region));
	game_copy(save, region, temp);

	complete_block_t block;
	save_info_get_current_brush_block(&block);
	for (int y = 0; y < region_width(region); y++)
	{
		for (int x = 0; x < region_height(region); x++)
		{
			tool_brush_add_to_before_list_cb(brush, &temp[x + y * region_width(region)]);
			game_paste(save, (region_t) { x + region.x0, y + region.y0, x + region.x0, y + region.y0 }, &block);
		}
	}

	free(temp);
}

static void save_viewer_brush_handle_mouse_button(tool_brush_t brush, bool m1_down, int x, int y)
{
	tool_event_t result = tool_brush_handle_mouse_click(brush, m1_down, x, y, y_pos);
	if (result == EVENT_BRUSH_END)
	{
		region_t region = tool_brush_region(brush);
		complete_block_t* temp = dig_malloc(region_size(region) * sizeof * temp * 2);

		tool_brush_copy_before_cb(brush, save, temp);
		game_copy(save, region, temp + region_size(region));

		action_buffer_add_block(action_buffer, temp, temp + region_size(region), region);

		free(temp);
	}
}

static void save_viewer_brush_handle_mouse_move(tool_brush_t brush, bool m1_down, int x, int y)
{
	tool_event_t result = tool_brush_handle_mouse_move(brush, m1_down, x, y, y_pos);
	if (!region_is_invalid(tool_brush_region(brush)))
	{
		save_viewer_invalidate_region(tool_brush_region(brush));
		screen_repaint();
	}
}

static void save_viewer_handle_mouse_button(bool m1_down, int x, int y)
{
	info_tool_t current = save_info_get_current_tool();
	if (current == TOOL_SELECT)
	{
		save_viewer_select_handle_mouse_button(m1_down, x, y);
	}
	else if (current == TOOL_ERASER)
	{
		save_viewer_brush_handle_mouse_button(eraser_tool, m1_down, x, y);
	}
	else if (current == TOOL_BRUSH)
	{
		save_viewer_brush_handle_mouse_button(brush_tool, m1_down, x, y);
	}
}

static void save_viewer_handle_mouse_move(bool m1_down, int x, int y)
{
	info_tool_t current = save_info_get_current_tool();
	if (current == TOOL_SELECT)
	{
		save_viewer_select_handle_mouse_move(m1_down, x, y);
	}
	else if (current == TOOL_ERASER)
	{
		save_viewer_brush_handle_mouse_move(eraser_tool, m1_down, x, y);
	}
	else if (current == TOOL_BRUSH)
	{
		save_viewer_brush_handle_mouse_move(brush_tool, m1_down, x, y);
	}
}

static void save_viewer_handle_mouse_wheel(int delta)
{
	save_viewer_move_window(delta * 10);
}

static void save_viewer_handle_block_change(region_t region)
{
	region = region_validate(region);
	save_viewer_invalidate_region(region);
	screen_repaint();
}

static void save_viewer_handle_global_field_change(const void* field)
{
	int layer_index;
	for (layer_index = 0; layer_index < LAYER_COUNT; layer_index++)
	{
		if (field == &save->layer_headers[layer_index].dirt_color)
		{
			break;
		}
	}
	if (layer_index < 0 || layer_index >= LAYER_COUNT)
	{
		return;
	}
	screen_sprite_set_dirt_color(cache[layer_index], *(rgb_color_t*)field);
	int mid = (y_pos + TARGET_HEIGHT / 2) / TARGET_HEIGHT;
	if (mid == layer_index)
	{
		screen_change_dirt_color(screen_sprite_dirt_color(cache[layer_index]));
	}
	screen_repaint();
}

static void save_viewer_handle_tool_change(info_tool_t next_tool)
{
	tool_select_reset(select_tool);
	if (cache[0])
	{
		screen_repaint();
	}
}

static void save_viewer_handle_brush_size_change(int size)
{
	tool_brush_set_size(brush_tool, size);
	tool_brush_set_size(eraser_tool, size);
}

void save_initialize(editor_state_t* state)
{
	editor = state;

	info_mode_class_t class =
	{
		.mode = MODE_SAVE,
		.caption = "Save",
		.initialize = save_info_initialize,
		.destroy = save_info_destroy,
		.show = save_info_show,
		.proc = save_info_proc,
		.interact_tree_item = save_info_handle_interact_tree_item
	};
	info_add_class(&class);

	action_buffer = action_buffer_initialize();
	save_info_action_buffer_set(action_buffer);

	char buf[MAX_PATH];
	asset_t asset = file_asset_load(path_find_dnr_main(buf, sizeof buf, "Sprites\\Checkpoint.sprite"));
	flag = game_spritify_asset(asset);
	file_asset_unload(&asset);
	RUNTIME_ASSERT(flag);

	select_tool = tool_select_create(WORLD_WIDTH, WORLD_HEIGHT);
	eraser_tool = tool_brush_create(save_viewer_erase, BRUSH_TYPE_COMPLETE_BLOCK, WORLD_WIDTH, WORLD_HEIGHT);
	brush_tool = tool_brush_create(save_viewer_brush, BRUSH_TYPE_COMPLETE_BLOCK, WORLD_WIDTH, WORLD_HEIGHT);
}

void save_destroy(void)
{
	screen_sprite_destroy(flag);
	file_state_unload(save);

	free(clipboard_data);

	action_buffer_destroy(action_buffer);
	tool_brush_destroy(eraser_tool);
	tool_brush_destroy(brush_tool);
	tool_select_destroy(select_tool);
}

static bool save_try_load_save(void)
{
	debug_format("Trying to load save\n");
	save = file_state_load(path_find_dnr_save(save_directory, sizeof save_directory, editor->current_save));
	if (!save)
	{
		save_viewer_prompt_which_save();
		file_editor_save(editor);
		save = file_state_load(path_find_dnr_save(save_directory, sizeof save_directory, editor->current_save));
		if (!save)
		{
			debug_format("Failed to open a save file on launch\n");
			return false;
		}
	}
	return true;
}

void save_start(void)
{
	screen_events_t screen_events =
	{
		.repaint = save_viewer_handle_repaint,
		.keyboard = save_viewer_handle_keyboard,
		.mouse_button = save_viewer_handle_mouse_button,
		.mouse_move = save_viewer_handle_mouse_move,
		.mouse_wheel = save_viewer_handle_mouse_wheel
	};
	screen_set_event_handlers(&screen_events);

	info_events_t info_events =
	{
		.block_handler = save_viewer_handle_block_change,
		.global_field_handler = save_viewer_handle_global_field_change,
		.tool_handler = save_viewer_handle_tool_change,
		.brush_size_handler = save_viewer_handle_brush_size_change
	};
	info_set_event_handlers(&info_events);

	if (!save && !save_try_load_save())
	{
		/* eventually, this shouldn't crash the application */
		RUNTIME_ASSERT(false);
		return;
	}
	RUNTIME_ASSERT(save);
	for (int i = 0; i < LAYER_COUNT; i++)
	{
		cache[i] = game_spritify_layer(save, i);
	}

	save_viewer_move_window(TARGET_HEIGHT / 2 - save->spawn_y);
	save_info_state_set(save); /* wait till last second to call so that there's not much waiting if any on this thread */
}

void save_end(void)
{
	tool_brush_reset(brush_tool);
	tool_brush_reset(eraser_tool);
	tool_select_reset(select_tool);
	save_viewer_move_window(y_pos);
}