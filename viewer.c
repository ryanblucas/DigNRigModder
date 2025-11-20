/*
	viewer.c ~ RL

	Views all Dig-N-Rig sprites found in the game's directory.
*/

#include "file.h"
#include "screen.h"
#include <stdio.h>
#include <Windows.h>

/* Temporary, you'd need to actually find this programatically but it'll work most of the time */
#define DIG_N_RIG_SPRITE_PATH "C:\\Program Files (x86)\\DigiPen\\Dig-N-Rig\\Sprites\\"
#define DIG_N_RIG_LAYER_PATH "C:\\Program Files (x86)\\DigiPen\\Dig-N-Rig\\Layers\\"

static sprite_t current;

static int index;
static bool is_viewing_sprites;

/* there are 472 sprites in Dig-N-Rig, but just to be safe, we'll do 512... */
static char* sprite_directories[512];
static int sprite_directory_count;
/* there are 32 layers in Dig-N-Rig, but again just to be safe, we'll do 64 */
static char* layer_directories[64];
static int layer_directory_count;

static void viewer_reload_sprite(void)
{
	char** directories = is_viewing_sprites ? sprite_directories : layer_directories;
	sprite_t next = file_load_sprite(directories[index]);
	char buf[MAX_PATH + 27];
	if (!next)
	{
		snprintf(buf, sizeof buf, "\"%s\" - Failed to load!", directories[index]);
		screen_change_title(buf);
		return;
	}
	screen_sprite_destroy(current);
	current = next;

	snprintf(buf, sizeof buf, "\"%s\" - Width: %i, Height: %i", directories[index], screen_sprite_width(current), screen_sprite_height(current));
	screen_change_title(buf);
	screen_repaint();
}

void viewer_handle_repaint()
{
	screen_sprite_render(TARGET_WIDTH / 2 - screen_sprite_width(current) / 2, TARGET_HEIGHT / 2 - screen_sprite_height(current) / 2, current);
}

void viewer_handle_keyboard(virtual_key_t vk)
{
	int dir_count = is_viewing_sprites ? sprite_directory_count : layer_directory_count;
	switch (vk)
	{
	case VK_LEFT:
		index = ((index + dir_count) - 1) % dir_count;
		viewer_reload_sprite();
		break;
	case VK_RIGHT:
		index = (index + 1) % dir_count;
		viewer_reload_sprite();
		break;
	case 'S':
		is_viewing_sprites = !is_viewing_sprites;
		index = 0;
		viewer_reload_sprite();
		break;
	}
}

static int viewer_initialize_directories(const char* base, char** directories, size_t directory_count)
{
	int count = 0;

	WIN32_FIND_DATAA ffd;
	char base_buf[MAX_PATH];
	snprintf(base_buf, sizeof base_buf, "%s*", base);
	HANDLE find = FindFirstFileA(base_buf, &ffd);
	if (find == INVALID_HANDLE_VALUE)
	{
		debug_format("Failed to locate Dig-N-Rig.\n");
		exit(1);
	}

	size_t size_of_base = strnlen(base, MAX_PATH);
	do
	{
		if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			/* plus one for null terminator */
			size_t dir_len = strnlen(ffd.cFileName, sizeof ffd.cFileName) + size_of_base + 1;
			directories[count] = dig_malloc(dir_len);
			snprintf(directories[count], dir_len, "%s%s", base, ffd.cFileName);
			count++;
		}
	} while (FindNextFileA(find, &ffd) && directory_count > count);

	if (count > directory_count)
	{
		debug_format("Ran out of space to store the rest of the directories\n");
	}

	FindClose(find);

	return count;
}

static void viewer_initialize(void)
{
	sprite_directory_count = viewer_initialize_directories(DIG_N_RIG_SPRITE_PATH, sprite_directories, sizeof sprite_directories / sizeof * sprite_directories);
	layer_directory_count = viewer_initialize_directories(DIG_N_RIG_LAYER_PATH, layer_directories, sizeof layer_directories / sizeof * layer_directories);

	viewer_reload_sprite();
}

static void viewer_destroy(void)
{
	screen_sprite_destroy(current);
	for (int i = 0; i < sizeof sprite_directories / sizeof * sprite_directories; i++)
	{
		free(sprite_directories[i]);
	}
}

int main()
{
	screen_initialize((screen_events_t) { viewer_handle_repaint, viewer_handle_keyboard });
	viewer_initialize();
	
	screen_loop();

	screen_destroy();

	return 0;
}