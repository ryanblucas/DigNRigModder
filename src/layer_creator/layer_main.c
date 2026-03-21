/*
	layer_main.c ~ RL
*/

#include "layer_main.h"
#include "layer_info.h"
#include "../info_box.h"
#include "../screen.h"

static sprite_t cache;

void layer_initialize(editor_state_t* state)
{
	info_mode_class_t class =
	{
		.mode = MODE_LAYER,
		.caption = "Layer",
		.initialize = layer_info_initialize,
		.destroy = layer_info_destroy,
		.show = layer_info_show,
		.proc = layer_info_proc
	};
	info_add_class(&class);
}

void layer_destroy(void)
{
	screen_sprite_destroy(cache);
}

static void layer_handle_file_change(const char* directory)
{
	screen_sprite_destroy(cache);
	cache = file_sprite_load(directory);
	RUNTIME_ASSERT(cache);
	screen_repaint();
}

static void layer_handle_repaint(void)
{
	if (cache)
	{
		screen_change_dirt_color(screen_sprite_dirt_color(cache));
		screen_sprite_render(0, 0, cache);
	}
}

void layer_start(void)
{
	info_events_t info_events =
	{
		.file_handler = layer_handle_file_change
	};
	info_set_event_handlers(&info_events);
	screen_events_t screen_events =
	{
		.repaint = layer_handle_repaint
	};
	screen_set_event_handlers(&screen_events);
}

void layer_end(void)
{

}