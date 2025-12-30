/*
	save_viewer.c ~ RL

	Views save files
*/

#if 1
#include "file.h"
#include "screen.h"

static save_t* save;
static int y_pos;

void save_viewer_handle_repaint()
{
	int top = y_pos / TARGET_HEIGHT;
	int bottom = y_pos / TARGET_HEIGHT + 1;
	screen_sprite_render(0, -y_pos % TARGET_HEIGHT, save->layer_images[top]);
	screen_sprite_render(0, TARGET_HEIGHT - y_pos % TARGET_HEIGHT, save->layer_images[bottom]);
}

void save_viewer_handle_mouse_wheel(int delta)
{
	y_pos -= delta * 4;
	y_pos = min(y_pos, TARGET_HEIGHT * 13 - 1);
	y_pos = max(y_pos, 0);
	screen_repaint();
}

int main()
{
	screen_initialize((screen_events_t) { .repaint = save_viewer_handle_repaint, .mouse_wheel = save_viewer_handle_mouse_wheel });
	/* temporary */
	save = file_load_save("C:\\Users\\fcsto\\OneDrive\\Documents\\DigiPen\\Dig-N-Rig\\profile1.sav");
	if (!save)
	{
		return 1;
	}

	screen_loop();

	file_unload_save(save);
	screen_destroy();

	return 0;
}
#endif