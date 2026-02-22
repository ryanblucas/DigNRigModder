/*
	save_viewer.c ~ RL
	Views save files
*/

#include "file.h"
#include "info_box.h"
#include "screen.h"

static sprite_t flag;
static sprite_t cache[LAYER_COUNT];
static dnr_state_t* save;
static int y_pos;

static int selected_x = -1, selected_y = -1;

static void save_viewer_set_selected(int x, int y)
{
	if (!dig_inside_bounds(x, y))
	{
		selected_x = selected_y = -1;
		return;
	}

	selected_x = x;
	selected_y = y;

	if (y < y_pos || y >= y_pos + TARGET_HEIGHT)
	{
		y_pos = min(max(0, y - 16), TARGET_HEIGHT * 13 - 1);
	}

	info_cell_set_current(selected_x, selected_y);
	screen_repaint();
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

void save_viewer_handle_repaint()
{
	int top = y_pos / TARGET_HEIGHT;
	int bottom = y_pos / TARGET_HEIGHT + 1;
	screen_sprite_render(0, -y_pos % TARGET_HEIGHT, cache[top]);
	screen_sprite_render(0, TARGET_HEIGHT - y_pos % TARGET_HEIGHT, cache[bottom]);

	if (save->player.y_spawn > top * TARGET_HEIGHT && save->player.y_spawn < (bottom + 1) * TARGET_HEIGHT)
	{
		screen_sprite_render((int)save->player.x_spawn, (int)save->player.y_spawn - y_pos, flag);
	}

	attribute_t selected;
	screen_get_attrib_region(&selected, selected_x, selected_y - y_pos, 1, 1);
	selected = ~selected & 0xFF;
	screen_set_attrib_region(&selected, selected_x, selected_y - y_pos, 1, 1);
}

void save_viewer_handle_keyboard(virtual_key_t vk)
{
	if (vk == 'R')
	{
		debug_format("Reloading save...\n");
		file_state_unload(save);
		save = file_state_load("C:\\Users\\fcsto\\OneDrive\\Documents\\DigiPen\\Dig-N-Rig\\profile1.sav");

		for (int i = 0; i < LAYER_COUNT; i++)
		{
			cache[i] = file_state_spritify(save, i);
		}

		info_state_set(save);
		screen_repaint();
	}
	else if (vk == VK_UP)
	{
		save_viewer_move_window(1);
	}
	else if (vk == VK_DOWN)
	{
		save_viewer_move_window(-1);
	}
}

void save_viewer_handle_mouse_button(int x, int y)
{
	int new_selected_x = x;
	int new_selected_y = y + y_pos;

	if (new_selected_x == selected_x && new_selected_y == selected_y)
	{
		selected_x = selected_y = -1;
		info_cell_set_current(selected_x, selected_y);
		screen_repaint();
		return;
	}
	selected_x = new_selected_x;
	selected_y = new_selected_y;
	info_cell_set_current(selected_x, selected_y);

	screen_repaint();

	int block_index = (selected_x * WORLD_HEIGHT + selected_y);
	debug_format("%i, %i\n", selected_x, selected_y);
	debug_format("Cheat Engine block: \"Dig-N-Rig.exe\"+%X 0x54 array size\n", block_index * SAVE_BLOCK_STRUCT_SIZE + 0x661E00);
	if (save->blocks[block_index].mineral_exists && save->blocks[block_index].mineral_index >= 0 
		&& save->blocks[block_index].mineral_index < sizeof save->minerals / sizeof * save->minerals)
	{
		debug_format("Cheat Engine mineral: \"Dig-N-Rig.exe\"+%X 0x34 array size\n", save->blocks[block_index].mineral_index * SAVE_MINERAL_STRUCT_SIZE + 0x61890);
	}
	debug_format("Cheat Engine screen: \"Dig-N-Rig.exe\"+65C5CC pointer with offset 0x%X\n", (x + y * WORLD_WIDTH) * 4);
}

void save_viewer_handle_mouse_wheel(int delta)
{
	save_viewer_move_window(delta * 10);
}

int main()
{
	debug_profiler_push();

	screen_initialize((screen_events_t)
	{
		.repaint = save_viewer_handle_repaint,
		.keyboard = save_viewer_handle_keyboard,
		.mouse_button = save_viewer_handle_mouse_button,
		.mouse_wheel = save_viewer_handle_mouse_wheel
	});

	screen_change_title("Dig-N-Rig Display");

	info_initialize(NULL);

	debug_profiler_push();
	/* paths are temporary */
	save = file_state_load("C:\\Users\\fcsto\\OneDrive\\Documents\\DigiPen\\Dig-N-Rig\\profile1.sav");
	flag = file_sprite_load("C:\\Program Files (x86)\\DigiPen\\Dig-N-Rig\\Sprites\\Checkpoint.sprite");
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

	screen_loop();

	screen_sprite_destroy(flag);
	file_state_unload(save);
	info_destroy();
	screen_destroy();

	return 0;
}