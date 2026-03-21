/*
	main.c ~ RL
*/

#include "save_viewer/save_main.h"
#include "layer_creator/layer_main.h"
#include "screen.h"
#include "info_box.h"

static void call_respective_start(info_mode_t mode)
{
	switch (mode)
	{
	case MODE_SAVE:
		save_start();
		break;
	case MODE_LAYER:
		layer_start();
		break;
	}
}

static void call_respective_end(info_mode_t mode)
{
	switch (mode)
	{
	case MODE_SAVE:
		save_end();
		break;
	case MODE_LAYER:
		layer_end();
		break;
	}
}

static void change_mode_handler(info_mode_t mode)
{
	call_respective_end(info_get_current_mode());
	screen_clear();
	screen_invalidate();
	call_respective_start(mode);
}

int main()
{
	editor_state_t editor;
	if (!file_editor_load(&editor))
	{
		editor.current_save = 1;
	}

	save_initialize(&editor);
	layer_initialize(&editor);

	screen_initialize();
	screen_change_title("Dig-N-Rig Display");
	/* initialize info after screen so that the window is placed adjacent to the console as opposed to under it */
	info_initialize(change_mode_handler);

	call_respective_start(info_get_current_mode());
	screen_loop();
	call_respective_end(info_get_current_mode());

	save_destroy();
	layer_destroy();

	info_destroy();
	screen_destroy();
	return 0;
}