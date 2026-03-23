/*
	main.c ~ RL
*/

#include "save_viewer/save_main.h"
#include "layer_creator/layer_main.h"
#include "screen.h"
#include "info_box.h"

static editor_state_t editor;

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
	debug_profiler_push();

	call_respective_end(info_get_current_mode());
	screen_clear();
	screen_invalidate();
	editor.current_mode = mode;
	call_respective_start(mode);

	debug_profiler_pop("Mode change");
}

int main()
{
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
	info_set_current_mode(editor.current_mode);

	call_respective_start(info_get_current_mode());
	screen_loop();
	/* Actually, don't call the respective end. That's really just cleaning up for the next mode, which can cause problems at this point in execution.
	   For example, if the user stops the application from the info box in the save mode, the console is freed then screen_loop returns for this
	   function to call save_end. save_end will try to move the console up to the top, which it obviously cannot do because the console was freed earlier. */

	file_editor_save(&editor);

	save_destroy();
	layer_destroy();

	info_destroy();
	screen_destroy();
	return 0;
}