/*
	save_viewer.c ~ RL
	Views save files
*/

#include "action_buffer.h"
#include "change_field_modal.h"
#include "file.h"
#include "info_box.h"
#include "path.h"
#include "screen.h"
#include <stdio.h>

#define MAX_SELECTION_WIDTH 120
#define MAX_SELECTION_HEIGHT 80
#define MAX_SELECTION_SIZE (MAX_SELECTION_WIDTH * MAX_SELECTION_HEIGHT)

static void save_viewer_handle_global_field_change(const void* field);

static sprite_t flag;
static sprite_t cache[LAYER_COUNT];
static dnr_state_t* save;
static int y_pos;

static region_t selection_region;

static char save_directory[MAX_PATH];
static editor_state editor;

static void save_viewer_prompt_which_save(void)
{
	bool valid_save = false;
	while (editor.current_save <= 0 || editor.current_save >= 4 || !valid_save)
	{
		/* temporary use of change field modal function */
		change_field_modal_integer(NULL, &editor.current_save, sizeof editor.current_save | SIZE_IS_SIGNED);
		char directory[MAX_PATH];
		path_find_dnr_save(directory, sizeof directory, editor.current_save);
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

	region_t screen_region = region_validate(selection_region);
	screen_region.y0 -= y_pos;
	screen_region.y1 -= y_pos;
	
	RUNTIME_ASSERT(region_width(screen_region) <= MAX_SELECTION_WIDTH && region_height(screen_region) <= MAX_SELECTION_HEIGHT);

	attribute_t selected[MAX_SELECTION_SIZE];
	screen_get_attrib_region(selected, screen_region);
	for (int i = 0; i < region_size(screen_region); i++)
	{
		selected[i] = ~selected[i] & 0xFF;
	}
	screen_set_attrib_region(selected, screen_region);
}

static void save_viewer_delete_selection(void)
{
	debug_profiler_push();
	region_t region = region_validate(selection_region);
	action_buffer_pre_add_block(save, region);
	for (int x = region.x0; x <= region.x1; x++)
	{
		for (int y = region.y0; y <= region.y1; y++)
		{
			dnr_block_t* block = &save->blocks[x * WORLD_HEIGHT + y];
			block->health_current = 0;
			block->visual.Attributes = 0;
			block->visual.Char.AsciiChar = ' ';
			block->block_exists = false;
			block->rig_type = RIG_NONE;
			block->mineral_move_direction = MOVE_DIRECTION_DOWN;
			if (block->mineral_exists)
			{
				save->minerals[block->mineral_index].exists = false;
				block->mineral_exists = false;
				block->mineral_index = -1;
			}
			for (int i = 0; i < save->stalactite_count; i++)
			{
				if (save->stalactite_array[i].exists && (int)save->stalactite_array[i].x == x && (int)save->stalactite_array[i].y == y)
				{
					save->stalactite_array[i].exists = false;
				}
			}
		}
	}
	action_buffer_post_add_block(save);
	for (int y = region.y0 / TARGET_HEIGHT; y <= region.y1 / TARGET_HEIGHT; y++)
	{
		cache[y] = file_state_spritify(save, y);
	}
	selection_region = INVALID_REGION;
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
	int top = act->sub.block.region.y0 / TARGET_HEIGHT;
	int bottom = act->sub.block.region.y1 / TARGET_HEIGHT;
	for (; top <= bottom; top++)
	{
		cache[top] = file_state_spritify(save, top);
	}
	screen_repaint();
}

static void save_viewer_handle_keyboard(virtual_key_t vk, keyboard_control_t ctrl)
{
	if (ctrl & CTRL_LEFT_PRESSED)
	{
		if (vk == 'S')
		{
			debug_format("Saving to disk...\n");
			file_state_save(save_directory, save);
		}
		else if (vk == 'R')
		{
			if (ctrl & CTRL_SHIFT_PRESSED)
			{
				save_viewer_prompt_which_save();
				path_find_dnr_save(save_directory, sizeof save_directory, editor.current_save);
			}
			debug_format("Reloading save...\n");
			file_state_unload(save);
			save = file_state_load(save_directory);

			for (int i = 0; i < LAYER_COUNT; i++)
			{
				cache[i] = file_state_spritify(save, i);
			}

			info_state_set(save);
			screen_repaint();
		}
		else if (vk == 'Z')
		{
			save_viewer_do_action(action_buffer_back());
		}
		else if (vk == 'Y')
		{
			save_viewer_do_action(action_buffer_forward());
		}
	}
	else if (vk == VK_UP)
	{
		save_viewer_move_window(1);
	}
	else if (vk == VK_DOWN)
	{
		save_viewer_move_window(-1);
	}
	else if (vk == VK_DELETE)
	{
		save_viewer_delete_selection();
	}
}

static void save_viewer_handle_mouse_button(bool m1_down, int x, int y)
{
	if (!m1_down)
	{
		if (!region_is_invalid(selection_region) && region_size(selection_region) > 1)
		{
			info_cell_set_current_region(selection_region);
			screen_repaint();
		}
		return;
	}

	int new_selected_x = x;
	int new_selected_y = y + y_pos;

	if (new_selected_x == selection_region.x0 && new_selected_y == selection_region.y0 && region_size(selection_region) == 1)
	{
		info_cell_set_current(-1, -1);
		selection_region = INVALID_REGION;
		screen_repaint();
		return;
	}
	
	selection_region.x0 = selection_region.x1 = new_selected_x;
	selection_region.y0 = selection_region.y1 = new_selected_y;
	info_cell_set_current(new_selected_x, new_selected_y);

	screen_repaint();
}

static void save_viewer_handle_mouse_move(bool m1_down, int x, int y)
{
	if (!m1_down || region_is_invalid(selection_region))
	{
		return;
	}

	int new_selected_x = x;
	int new_selected_y = y + y_pos;
	selection_region.x1 = min(new_selected_x, selection_region.x0 + MAX_SELECTION_WIDTH - 1);
	selection_region.y1 = min(new_selected_y, selection_region.y0 + MAX_SELECTION_HEIGHT - 1);
	selection_region.x1 = max(selection_region.x1, selection_region.x0 - MAX_SELECTION_WIDTH + 1);
	selection_region.y1 = max(selection_region.y1, selection_region.y0 - MAX_SELECTION_HEIGHT + 1);
	if (region_size(selection_region) != 1)
	{
		info_cell_set_current(-1, -1);
	}
	screen_repaint();
}

static void save_viewer_handle_mouse_wheel(int delta)
{
	save_viewer_move_window(delta * 10);
}

static void save_viewer_handle_block_change(region_t region)
{
	region = region_validate(region);
	int layer_min = region.y0 / TARGET_HEIGHT;
	int layer_max = region.y1 / TARGET_HEIGHT;
	if (layer_min < 0 || layer_min >= LAYER_COUNT || layer_max < 0 || layer_max >= LAYER_COUNT)
	{
		return;
	}
	for (int i = layer_min; i <= layer_max; i++)
	{
		cache[i] = file_state_spritify(save, i);
	}
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

int main()
{
	if (!file_editor_load(&editor))
	{
		editor.current_save = 1;
		save_viewer_prompt_which_save();
		file_editor_save(&editor);
	}

	debug_profiler_push();

	screen_initialize((screen_events_t)
	{
		.repaint = save_viewer_handle_repaint,
		.keyboard = save_viewer_handle_keyboard,
		.mouse_button = save_viewer_handle_mouse_button,
		.mouse_move = save_viewer_handle_mouse_move,
		.mouse_wheel = save_viewer_handle_mouse_wheel
	});

	screen_change_title("Dig-N-Rig Display");

	info_initialize((info_events_t)
	{
		.mode_handler = NULL,
		.block_handler = save_viewer_handle_block_change,
		.global_field_handler = save_viewer_handle_global_field_change,
	});

	action_buffer_initialize();
	
	debug_profiler_push();

	char buf[MAX_PATH];
	save = file_state_load(path_find_dnr_save(save_directory, sizeof save_directory, 1));
	flag = file_sprite_load(path_find_dnr_main(buf, sizeof buf, "Sprites\\Checkpoint.sprite"));
	if (!save || !flag)
	{
		return 1;
	}

	debug_profiler_pop("Load assets");

	debug_profiler_push();
	for (int i = 0; i < LAYER_COUNT; i++)
	{
		cache[i] = file_state_spritify(save, i);
	}
	debug_profiler_pop("Spritify all layers");

	save_viewer_move_window(TARGET_HEIGHT / 2 - (int)save->player.y_spawn);
	info_state_set(save); /* wait till last second to call so that there's not much waiting if any on this thread */

	debug_profiler_pop("Application initialization");

	selection_region = INVALID_REGION;
	screen_loop();

	file_editor_save(&editor);

	screen_sprite_destroy(flag);
	file_state_unload(save);

	action_buffer_destroy();
	info_destroy();
	screen_destroy();

	return 0;
}