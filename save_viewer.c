/*
	save_viewer.c ~ RL
	Views save files
*/

#include "action_buffer.h"
#include "change_field_modal.h"
#include "file.h"
#include "game.h"
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
static int hinge_x = -1, hinge_y = -1;
static int move_x = -1, move_y = -1;
static sprite_t selection_visual;

static char save_directory[MAX_PATH];
static editor_state editor;

static inline void save_viewer_invalidate_region(region_t region)
{
	region = region_validate(region);
	for (int y = region.y0 / TARGET_HEIGHT; y <= region.y1 / TARGET_HEIGHT; y++)
	{
		cache[y] = file_state_spritify(save, y);
	}
}

static void save_viewer_start_move(int new_selected_x, int new_selected_y)
{
	region_t correct = region_validate(selection_region);
	hinge_x = new_selected_x - correct.x0;
	hinge_y = new_selected_y - correct.y0;
	move_x = new_selected_x;
	move_y = new_selected_y;

	char* text = dig_malloc(region_size(selection_region));
	attribute_t* attributes = dig_malloc(region_size(selection_region) * sizeof * attributes);

	correct.y0 -= y_pos;
	correct.y1 -= y_pos;
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

static void save_viewer_stop_move(void)
{
	region_t region = region_validate(selection_region);
	region_t total = region_merge(region,
		(region_t) {move_x - hinge_x, move_y - hinge_y, move_x - hinge_x + region_width(region), move_y - hinge_y + region_height(region)
	});
	action_buffer_pre_add_block(save, total);

	dnr_block_t* blocks = dig_malloc(region_size(region) * sizeof * blocks);
	for (int y = 0; y < region_height(region); y++)
	{
		for (int x = 0; x < region_width(region); x++)
		{
			int adj_x = x + region.x0;
			int adj_y = y + region.y0;
			
			dnr_block_t* old = game_get_block(save, adj_x, adj_y);
			dnr_block_t* new = &blocks[x + y * region_width(region)];
			
			*new = *old;
			new->x = move_x - hinge_x + x;
			new->y = move_y - hinge_y + y;
			
			game_delete_block_partial(save, adj_x, adj_y);
			
			dnr_mineral_t* mineral = game_get_mineral(save, adj_x, adj_y);
			if (mineral)
			{
				mineral->x -= old->x - new->x;
				mineral->y -= old->y - new->y;
			}
			stalactite_t* stalactite = game_get_stalactite(save, adj_x, adj_y);
			if (stalactite)
			{
				stalactite->x -= old->x - new->x;
				stalactite->y -= old->y - new->y;
			}
		}
	}
	for (int y = 0; y < region_height(region); y++)
	{
		for (int x = 0; x < region_width(region); x++)
		{
			int adj_x = x + move_x - hinge_x;
			int adj_y = y + move_y - hinge_y;
			save->blocks[adj_y + adj_x * WORLD_HEIGHT] = blocks[x + y * region_width(region)];
		}
	}

	free(blocks);
	action_buffer_post_add_block(save);

	save_viewer_invalidate_region(total);

	hinge_x = hinge_y = -1;
	move_x = move_y = -1;
	selection_region = INVALID_REGION;
	screen_sprite_destroy(selection_visual);
	selection_visual = NULL;

	screen_repaint();
}

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

	attribute_t* selected = dig_malloc(region_size(screen_region) * sizeof * selected);
	if (hinge_x >= 0 && hinge_y >= 0)
	{
		memset(selected, 0, region_size(screen_region) * sizeof * selected);
		screen_set_attrib_region(selected, screen_region);
		screen_sprite_render(move_x - hinge_x, move_y - y_pos - hinge_y, selection_visual);
	}
	else
	{
		screen_get_attrib_region(selected, screen_region);
		for (int i = 0; i < region_size(screen_region); i++)
		{
			selected[i] = ~selected[i] & 0xFF;
		}
		screen_set_attrib_region(selected, screen_region);
	}
	free(selected);
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
			game_delete_block(save, x, y);
		}
	}
	action_buffer_post_add_block(save);
	save_viewer_invalidate_region(selection_region);
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
	save_viewer_invalidate_region(act->sub.block.region);
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
		if (hinge_x >= 0 && hinge_y >= 0)
		{
			save_viewer_stop_move();
		}
		if (!region_is_invalid(selection_region) && region_size(selection_region) > 1)
		{
			info_cell_set_current_region(selection_region);
			screen_repaint();
		}
		return;
	}

	int new_selected_x = x;
	int new_selected_y = y + y_pos;

	if (!region_is_invalid(selection_region) && region_is_inside(selection_region, new_selected_x, new_selected_y))
	{
		save_viewer_start_move(new_selected_x, new_selected_y);
		return;
	}

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

	if (hinge_x >= 0 && hinge_y >= 0)
	{
		move_x = new_selected_x;
		move_y = new_selected_y;
		screen_repaint();
		return;
	}

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