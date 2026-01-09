/*
	save_viewer.c ~ RL

	Views save files
*/

#if 1
#include "file.h"
#include "screen.h"

static sprite_t flag;
static save_t* save;
static int y_pos;

static int selected_x = -1, selected_y = -1;

static void save_viewer_move_window(int addend)
{
	static int prev_mid = 0;
	y_pos -= addend;
	y_pos = min(y_pos, TARGET_HEIGHT * 13 - 1);
	y_pos = max(y_pos, 0);
	int mid = (y_pos + TARGET_HEIGHT / 2) / TARGET_HEIGHT;
	if (prev_mid != mid)
	{
		screen_change_dirt_color(screen_sprite_dirt_color(save->layer_images[mid]));
	}
	prev_mid = mid;
	screen_repaint();
}

void save_viewer_handle_repaint()
{
	int top = y_pos / TARGET_HEIGHT;
	int bottom = y_pos / TARGET_HEIGHT + 1;
	screen_sprite_render(0, -y_pos % TARGET_HEIGHT, save->layer_images[top]);
	screen_sprite_render(0, TARGET_HEIGHT - y_pos % TARGET_HEIGHT, save->layer_images[bottom]);

	if (save->y_spawn > top * TARGET_HEIGHT && save->y_spawn < (bottom + 1) * TARGET_HEIGHT)
	{
		screen_sprite_render((int)save->x_spawn, (int)save->y_spawn - y_pos, flag);
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
		file_unload_save(save);
		save = file_load_save("C:\\Users\\fcsto\\OneDrive\\Documents\\DigiPen\\Dig-N-Rig\\profile1.sav");
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
		screen_repaint();
		return;
	}
	selected_x = new_selected_x;
	selected_y = new_selected_y;

	screen_repaint();

	debug_format("%i, %i\n", selected_x, selected_y);
	debug_format("Cheat Engine block: \"Dig-N-Rig.exe\"+%X 0x54 array size\n", (selected_x * TARGET_HEIGHT * 14 + selected_y) * SAVE_BLOCK_STRUCT_SIZE + 0x661E00);
	debug_format("Cheat Engine screen: \"Dig-N-Rig.exe\"+65C5CC pointer with offset 0x%X\n", (x + y * TARGET_WIDTH) * 4);
}

void save_viewer_handle_mouse_wheel(int delta)
{
	save_viewer_move_window(delta * 10);
}

int main()
{
	screen_initialize((screen_events_t)
	{
		.repaint = save_viewer_handle_repaint,
		.keyboard = save_viewer_handle_keyboard,
		.mouse_button = save_viewer_handle_mouse_button,
		.mouse_wheel = save_viewer_handle_mouse_wheel
	});

	/* temporary */
	save = file_load_save("C:\\Users\\fcsto\\OneDrive\\Documents\\DigiPen\\Dig-N-Rig\\profile1.sav");
	flag = file_load_sprite("C:\\Program Files (x86)\\DigiPen\\Dig-N-Rig\\Sprites\\Checkpoint.sprite");
	if (!save || !flag)
	{
		return 1;
	}

	screen_change_dirt_color(screen_sprite_dirt_color(save->layer_images[0]));
	screen_loop();

	screen_sprite_destroy(flag);
	file_unload_save(save);
	screen_destroy();

	return 0;
}
#endif