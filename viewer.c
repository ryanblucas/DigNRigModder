/*
	viewer.c ~ RL

	Views all Dig-N-Rig sprites found in the game's directory.
*/

#include "file.h"
#include "screen.h"
#include <stdio.h>
#include <Windows.h>

#define DIG_N_RIG_SPRITE_PATH "C:\\Program Files (x86)\\DigiPen\\Dig-N-Rig\\Sprites\\"

static sprite_t current;

static int index;
/* there are 472 sprites in Dig-N-Rig, but just to be safe, we'll do 512... */
static char* directories[512];
static int directory_count;

static void viewer_reload_sprite(void)
{
	sprite_t next = file_load_sprite(directories[index]);
	if (!next)
	{
		char buf[MAX_PATH + 21];
		snprintf(buf, sizeof buf, "\"%s\" - Failed to load!", directories[index]);
		screen_change_title(buf);
		return;
	}
	screen_sprite_destroy(current);
	current = next;
	char buf[MAX_PATH + 27];
	snprintf(buf, sizeof buf, "\"%s\" - Width: %i, Height: %i", directories[index], screen_sprite_width(current), screen_sprite_height(current));
	screen_change_title(buf);
	screen_repaint();
}

void viewer_handle_repaint()
{
	screen_sprite_render(TARGET_WIDTH / 2 - screen_sprite_width(current) / 2, TARGET_HEIGHT / 2 - screen_sprite_height(current) / 2, current);
}

void viewer_handle_left()
{
	index = ((index + directory_count) - 1) % directory_count;
	viewer_reload_sprite();
}

void viewer_handle_right()
{
	index = (index + 1) % directory_count;
	viewer_reload_sprite();
}

static void viewer_initialize(void)
{
	WIN32_FIND_DATAA ffd;
	HANDLE find = FindFirstFileA(DIG_N_RIG_SPRITE_PATH "*", &ffd);
	if (find == INVALID_HANDLE_VALUE)
	{
		debug_format("Failed to locate Dig-N-Rig.\n");
		exit(1);
	}

	do
	{
		if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			/* plus one for null terminator */
			int dir_len = strnlen(ffd.cFileName, sizeof ffd.cFileName) + sizeof DIG_N_RIG_SPRITE_PATH + 1;
			directories[directory_count] = dig_malloc(dir_len);
			snprintf(directories[directory_count], dir_len, DIG_N_RIG_SPRITE_PATH "%s", ffd.cFileName);
			directory_count++;
		}
	} while (FindNextFileA(find, &ffd));

	FindClose(find);

	viewer_reload_sprite();
}

static void viewer_destroy(void)
{
	screen_sprite_destroy(current);
	for (int i = 0; i < sizeof directories / sizeof * directories; i++)
	{
		free(directories[i]);
	}
}

int main()
{
	screen_initialize((screen_events_t) { viewer_handle_repaint, viewer_handle_left, viewer_handle_right });
	viewer_initialize();
	
	screen_loop();

	screen_destroy();

	return 0;
}