/*
	main.c ~ RL
*/

#include "save_viewer/save_main.h"
#include "screen.h"
#include "info_box.h"

int main()
{
	editor_state_t editor;
	if (!file_editor_load(&editor))
	{
		editor.current_save = 1;
	}

	save_initialize(&editor);

	screen_initialize();
	screen_change_title("Dig-N-Rig Display");
	/* initialize info after screen so that the window is placed adjacent to the console as opposed to under it */
	info_initialize();

	save_start();
	screen_loop();
	save_end();

	save_destroy();

	info_destroy();
	screen_destroy();
	return 0;
}