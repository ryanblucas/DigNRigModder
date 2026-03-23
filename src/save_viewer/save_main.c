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
#include <stdio.h>

#define MAX_SELECTION_WIDTH 120
#define MAX_SELECTION_HEIGHT 80
#define MAX_SELECTION_SIZE (MAX_SELECTION_WIDTH * MAX_SELECTION_HEIGHT)

static void save_viewer_handle_global_field_change(const void* field);

typedef void (*save_viewer_brush_function_t)(int x, int y, int radius);

static action_buffer_t action_buffer;

static sprite_t flag;
static sprite_t cache[LAYER_COUNT];
static dnr_state_t* save;
static int y_pos;

static int brush_before_reserved;
static int brush_before_size;
static complete_block_t* brush_before_ptr;
static region_t brush_region;

static region_t selection_region;
static int hinge_x = -1, hinge_y = -1;
static int move_x = -1, move_y = -1;
static sprite_t selection_visual;

static char save_directory[MAX_PATH];
static editor_state_t* editor;

static region_t clipboard_region;
static complete_block_t* clipboard_data;

static inline void save_viewer_invalidate_region(region_t region)
{
	region = region_validate(region);
	for (int y = region.y0 / TARGET_HEIGHT; y <= region.y1 / TARGET_HEIGHT; y++)
	{
		screen_sprite_destroy(cache[y]);
		cache[y] = game_spritify_layer(save, y);
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
		action_buffer_pre_add_block(action_buffer, save, total);

		complete_block_t* blocks = dig_malloc(region_size(src_region) * sizeof * blocks);
		game_copy(save, src_region, blocks);
		game_delete(save, src_region);
		game_paste(save, dest_region, blocks);
		free(blocks);

		action_buffer_post_add_block(action_buffer, save);

		save_viewer_invalidate_region(total);
	}

	hinge_x = hinge_y = -1;
	move_x = move_y = -1;
	selection_region = INVALID_REGION;
	screen_sprite_destroy(selection_visual);
	selection_visual = NULL;
	save_info_cell_set_current(-1, -1);

	screen_repaint();
}

static void save_viewer_copy(void)
{
	if (region_is_invalid(selection_region))
	{
		return;
	}

	if (clipboard_data)
	{
		free(clipboard_data);
	}

	clipboard_region = region_validate(selection_region);
	clipboard_data = dig_malloc(region_size(clipboard_region) * sizeof * clipboard_data);
	game_copy(save, clipboard_region, clipboard_data);
}

static void save_viewer_paste(void)
{
	if (!clipboard_data || region_is_invalid(selection_region))
	{
		return;
	}

	int origin_x = min(selection_region.x0, selection_region.x1);
	int origin_y = min(selection_region.y0, selection_region.y1);
	region_t dest = { origin_x, origin_y, origin_x + region_width(clipboard_region) - 1, origin_y + region_height(clipboard_region) - 1 };

	if (!dig_inside_bounds(dest.x0, dest.y0) || !dig_inside_bounds(dest.x1, dest.y1))
	{
		return;
	}

	action_buffer_pre_add_block(action_buffer, save, dest);

	game_delete(save, selection_region);
	game_paste(save, dest, clipboard_data);

	action_buffer_post_add_block(action_buffer, save);

	selection_region = dest;
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

	if (region_is_invalid(selection_region))
	{
		return;
	}

	region_t screen_region = region_validate(selection_region);
	screen_region.y0 -= y_pos;
	screen_region.y1 -= y_pos;
	if (hinge_x >= 0 && hinge_y >= 0)
	{
		attribute_t* selected = dig_malloc(region_size(screen_region) * sizeof * selected);
		memset(selected, 0, region_size(screen_region) * sizeof * selected);

		screen_set_attrib_region(selected, screen_region);
		screen_sprite_render(move_x - hinge_x, move_y - y_pos - hinge_y, selection_visual);
		free(selected);
		return;
	}
	screen_invert_region(region_validate(screen_region));
}

static void save_viewer_delete_selection(void)
{
	debug_profiler_push();
	region_t region = region_validate(selection_region);
	action_buffer_pre_add_block(action_buffer, save, region);

	game_delete(save, region);

	action_buffer_post_add_block(action_buffer, save);
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
		}
		else if (vk == 'Z')
		{
			save_viewer_do_action(action_buffer_back(action_buffer));
		}
		else if (vk == 'Y')
		{
			save_viewer_do_action(action_buffer_forward(action_buffer));
		}
		else if (vk == 'C')
		{
			save_viewer_copy();
		}
		else if (vk == 'V')
		{
			save_viewer_paste();
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

static void save_viewer_select_handle_mouse_button(bool m1_down, int x, int y)
{
	if (!m1_down)
	{
		if (hinge_x >= 0 && hinge_y >= 0)
		{
			save_viewer_stop_move();
		}
		if (!region_is_invalid(selection_region) && region_size(selection_region) > 1)
		{
			save_info_cell_set_current_region(selection_region);
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

	selection_region.x0 = selection_region.x1 = new_selected_x;
	selection_region.y0 = selection_region.y1 = new_selected_y;
	save_info_cell_set_current(new_selected_x, new_selected_y);

	screen_repaint();
}

static void save_viewer_select_handle_mouse_move(bool m1_down, int x, int y)
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
		save_info_cell_set_current(-1, -1);
	}
	screen_repaint();
}

static void save_viewer_brush_handle_mouse_button(bool m1_down, int x, int y)
{
	if (m1_down)
	{
		y += y_pos;
		int radius = save_info_get_current_brush_size() - 1;
		brush_region = region_keep_inside(
			(region_t) { x - radius, y - radius, x + radius, y + radius }, 
			(region_t) { 0, 0, WORLD_WIDTH - 1, WORLD_HEIGHT - 1 });
		return;
	}

	if (region_is_invalid(brush_region))
	{
		return;
	}

	complete_block_t* new = dig_malloc(region_size(brush_region) * sizeof * new * 2);
	game_copy(save, brush_region, new);
	brush_region = region_validate(brush_region);
	for (int i = 0; i < brush_before_size; i++)
	{
		complete_block_t* curr = brush_before_ptr + i;
		new[(curr->block.x - brush_region.x0) + (curr->block.y - brush_region.y0) * region_width(brush_region)] = *curr;
	}
	game_copy(save, brush_region, new + region_size(brush_region));
	action_buffer_add_block(action_buffer, new, new + region_size(brush_region), brush_region);
	free(new);

	brush_region = INVALID_REGION;
	brush_before_size = 0;
}

static void save_viewer_add_to_brush_list(const complete_block_t* block)
{
	for (int j = 0; j < brush_before_size; j++)
	{
		if (brush_before_ptr[j].block.x == block->block.x && brush_before_ptr[j].block.y == block->block.y)
		{
			return;
		}
	}

	brush_before_ptr[brush_before_size++] = *block;
	if (brush_before_size < brush_before_reserved)
	{
		return;
	}

	size_t buf_size = brush_before_reserved * sizeof * brush_before_ptr;
	brush_before_reserved *= 2;
	complete_block_t* next = dig_malloc(buf_size * 2);
	memcpy(next, brush_before_ptr, buf_size);
	free(brush_before_ptr);
	brush_before_ptr = next;
}

static void save_viewer_erase(int x, int y, int radius)
{
	region_t region = { x - radius, y - radius, x + radius, y + radius };
	region = region_keep_inside(region, (region_t) { 0, 0, WORLD_WIDTH - 1, WORLD_HEIGHT - 1 });

	complete_block_t* temp = dig_malloc(sizeof * temp * region_size(region));
	game_copy(save, region, temp);

	game_delete(save, region);

	for (int i = 0; i < region_size(region); i++)
	{
		save_viewer_add_to_brush_list(&temp[i]);
	}
	free(temp);
}

static void save_viewer_brush(int x, int y, int radius)
{
	region_t region = { x - radius, y - radius, x + radius, y + radius };
	region = region_keep_inside(region, (region_t) { 0, 0, WORLD_WIDTH - 1, WORLD_HEIGHT - 1 });

	complete_block_t* temp = dig_malloc(sizeof * temp * region_size(region));
	game_copy(save, region, temp);

	complete_block_t brush;
	save_info_get_current_brush_block(&brush);
	for (int y = 0; y < region_width(region); y++)
	{
		for (int x = 0; x < region_height(region); x++)
		{
			save_viewer_add_to_brush_list(&temp[x + y * region_width(region)]);
			game_paste(save, (region_t) { x + region.x0, y + region.y0, x + region.x0, y + region.y0 }, &brush);
		}
	}

	free(temp);
}

static void save_viewer_brush_handle_mouse_move(save_viewer_brush_function_t function, bool m1_down, int x, int y)
{
	static int prev_selected_x = -1;
	static int prev_selected_y = -1;

	if (!m1_down || region_is_invalid(brush_region))
	{
		prev_selected_x = prev_selected_y = -1;
		return;
	}

	int new_selected_x = x;
	int new_selected_y = y + y_pos;
	int radius = save_info_get_current_brush_size() - 1;

	region_t final;
	if (prev_selected_x == -1 && prev_selected_y == -1)
	{
		final = (region_t){ new_selected_x - radius, new_selected_y - radius, new_selected_x + radius, new_selected_y + radius };
		function(new_selected_x, new_selected_y, radius);
	}
	else
	{
		final = region_validate((region_t) { new_selected_x, new_selected_y, prev_selected_x, prev_selected_y });
		RUNTIME_ASSERT(dig_inside_bounds(final.x0, final.y0) && dig_inside_bounds(final.x1, final.y1));
		final.x0 -= radius;
		final.y0 -= radius;
		final.x1 += radius;
		final.y1 += radius;
		final = region_keep_inside(final, (region_t) { 0, 0, WORLD_WIDTH - 1, WORLD_HEIGHT - 1 });
		brush_region = region_merge(brush_region, final);

		int cx = new_selected_x;
		int cy = new_selected_y;
		int dx = abs(cx - prev_selected_x);
		int dy = -abs(cy - prev_selected_y);
		int ix = cx < prev_selected_x ? 1 : -1;
		int iy = cy < prev_selected_y ? 1 : -1;
		
		int error = dx + dy;

		/* theres probably a way of doing this more efficiently for a line with a thickness */
		while (true)
		{
			function(cx, cy, radius);
			if (error * 2 <= dx)
			{
				if (cy == prev_selected_y)
				{
					break;
				}
				cy += iy;
				error += dx;
			}
			if (error * 2 >= dy)
			{
				if (cx == prev_selected_x)
				{
					break;
				}
				cx += ix;
				error += dy;
			}
		}
	}

	save_viewer_invalidate_region(final);
	screen_repaint();

	prev_selected_x = new_selected_x;
	prev_selected_y = new_selected_y;
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
		save_viewer_brush_handle_mouse_button(m1_down, x, y);
	}
	else if (current == TOOL_BRUSH)
	{
		save_viewer_brush_handle_mouse_button(m1_down, x, y);
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
		save_viewer_brush_handle_mouse_move(save_viewer_erase, m1_down, x, y);
	}
	else if (current == TOOL_BRUSH)
	{
		save_viewer_brush_handle_mouse_move(save_viewer_brush, m1_down, x, y);
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

void save_viewer_handle_tool_change(info_tool_t tool)
{
	hinge_x = hinge_y = -1;
	move_x = move_y = -1;
	selection_region = brush_region = INVALID_REGION;
	brush_before_size = 0;
	screen_sprite_destroy(selection_visual);
	selection_visual = NULL;

	if (cache[0])
	{
		screen_repaint();
	}
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
		.proc = save_info_proc
	};
	info_add_class(&class);

	action_buffer = action_buffer_initialize();
	save_info_action_buffer_set(action_buffer);

	char buf[MAX_PATH];
	asset_t asset = file_asset_load(path_find_dnr_main(buf, sizeof buf, "Sprites\\Checkpoint.sprite"));
	flag = game_spritify_asset(asset);
	file_asset_unload(&asset);
	RUNTIME_ASSERT(flag);
}

void save_destroy(void)
{
	screen_sprite_destroy(flag);
	file_state_unload(save);

	free(clipboard_data);
	free(brush_before_ptr);

	action_buffer_destroy(action_buffer);
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
	};
	info_set_event_handlers(&info_events);

	if (!save && !save_try_load_save())
	{
		/* eventually, this shouldn't crash the application */
		RUNTIME_ASSERT(false);
		return;
	}
	for (int i = 0; i < LAYER_COUNT; i++)
	{
		cache[i] = game_spritify_layer(save, i);
	}

	save_viewer_move_window(TARGET_HEIGHT / 2 - save->spawn_y);
	save_info_state_set(save); /* wait till last second to call so that there's not much waiting if any on this thread */

	selection_region = brush_region = INVALID_REGION;
	brush_before_reserved = MAX_SELECTION_SIZE;
	brush_before_ptr = dig_malloc(sizeof * brush_before_ptr * brush_before_reserved);
}

void save_end(void)
{
	selection_region = brush_region = INVALID_REGION;
	save_viewer_move_window(y_pos);
}