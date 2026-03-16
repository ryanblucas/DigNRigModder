/*
	main.c ~ RL
*/

#include "save_viewer/save_main.h"
#include "screen.h"

int main()
{
	save_initialize();
	screen_loop();
	save_destroy();
	return 0;
}