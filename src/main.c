/*
	main.c ~ RL
*/

#include "save_viewer/save_main.h"
#include "asset_creator/asset_main.h"
#include "campaign_creator/campaign_main.h"
#include "screen.h"
#include "info_box.h"

static editor_state_t editor;
static bool already_cleaned_up;

static void call_respective_start(info_mode_t mode)
{
	switch (mode)
	{
	case MODE_SAVE:
		save_start();
		break;
	case MODE_ASSET:
		asset_start();
		break;
	case MODE_CAMPAIGN:
		campaign_start();
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
	case MODE_ASSET:
		asset_end();
		break;
	case MODE_CAMPAIGN:
		campaign_end();
		break;
	}
}

static void change_mode_handler(info_mode_t old_mode)
{
	debug_profiler_push();

	call_respective_end(old_mode);
	screen_clear();
	screen_invalidate();
	editor.current_mode = info_get_current_mode();
	call_respective_start(editor.current_mode);

	debug_profiler_pop("Mode change");
}

static void cleanup(void)
{
	if (already_cleaned_up)
	{
		return;
	}
	already_cleaned_up = true;
	debug_profiler_push();

	file_editor_save(&editor);

	FreeConsole();
	info_destroy();
	screen_wait_for_end();

	screen_destroy();

	save_destroy();
	asset_destroy();
	campaign_destroy();

	debug_profiler_pop("Cleanup");
}

static BOOL WINAPI ctrl_handler(DWORD ctrl_type)
{
	if (ctrl_type == CTRL_CLOSE_EVENT)
	{
		debug_format("Closed from console\n");
		cleanup();
		return TRUE;
	}
	return FALSE;
}

int main()
{
	queue_set_main_thread_id(GetCurrentThreadId());
	SetConsoleCtrlHandler(ctrl_handler, TRUE);

	if (!file_editor_load(&editor))
	{
		debug_format("No editor config found, making new one\n");
		editor.current_save = 1;
		editor.max_events_per_frame = 5;
		editor.simulation_framerate = 30;
		editor.asset_palette[0] = (asset_block_t){ .visual = { .Char.AsciiChar = 0xDB, .Attributes = CREATE_ATTRIBUTE(DARK_YELLOW, DARK_BLACK) } };
		editor.asset_palette[1] = (asset_block_t){ .visual = { .Char.AsciiChar = 0xB0, .Attributes = CREATE_ATTRIBUTE(DARK_YELLOW, DARK_BLACK) } };
		editor.asset_palette[2] = (asset_block_t){ .visual = { .Char.AsciiChar = 0xB1, .Attributes = CREATE_ATTRIBUTE(DARK_YELLOW, DARK_BLACK) } };
	}

	if (editor.simulation_framerate >= 0 && editor.max_events_per_frame <= 0)
	{
		editor.max_events_per_frame = 1;
	}

	save_initialize(&editor);
	asset_initialize(&editor);
	campaign_initialize(&editor);

	screen_initialize(editor.is_small_console);
	screen_change_title("Dig-N-Rig Display");
	/* initialize info after screen so that the window is placed adjacent to the console as opposed to under it */
	info_initialize(change_mode_handler);
	info_mode_t mode = info_get_current_mode();
	info_set_current_mode(editor.current_mode);
	if (mode == editor.current_mode)
	{
		call_respective_start(info_get_current_mode());
	}
	screen_loop(editor.max_events_per_frame, editor.simulation_framerate);
	/* Actually, don't call the respective end. That's really just cleaning up for the next mode, which can cause problems at this point in execution.
	   For example, if the user stops the application from the info box in the save mode, the console is freed then screen_loop returns for this
	   function to call save_end. save_end will try to move the console up to the top, which it obviously cannot do because the console was freed earlier. */

	cleanup();
	return 0;
}