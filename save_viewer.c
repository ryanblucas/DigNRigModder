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
}

void save_viewer_handle_mouse_wheel(int delta)
{
	static int prev_mid = 0;
	y_pos -= delta * 10;
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

int main()
{
	screen_initialize((screen_events_t) { .repaint = save_viewer_handle_repaint, .keyboard = save_viewer_handle_keyboard, .mouse_wheel = save_viewer_handle_mouse_wheel });
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