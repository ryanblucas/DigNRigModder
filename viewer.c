/*
	viewer.c ~ RL

	Views all Dig-N-Rig sprites found in the game's directory.
*/

#include <assert.h>
#include "file.h"
#include "screen.h"
#include <stdio.h>
#include <Windows.h>

#define COUNT_OF(arr) (sizeof (arr) / sizeof * (arr))

/* Temporary, you'd need to actually find this programatically but it'll work most of the time */
#define DIG_N_RIG_SPRITE_PATH "C:\\Program Files (x86)\\DigiPen\\Dig-N-Rig\\Sprites\\"
#define DIG_N_RIG_LAYER_PATH "C:\\Program Files (x86)\\DigiPen\\Dig-N-Rig\\Layers\\"

enum viewer_mode
{
	MODE_SCROLL_LAYERS,
	MODE_VIEW_LAYERS,
	MODE_VIEW_SPRITES,
	MODE_COUNT
} mode;

/* for MODE_SCROLL_LAYERS, this variable is not used. */
static sprite_t current;

static int index;
static int scroll_speed = 1;

/* there are 472 sprites in Dig-N-Rig, but just to be safe, we'll do 512... */
static char* sprite_directories[512];
static int sprite_directory_count;
/* there are 32 layers in Dig-N-Rig, but again just to be safe, we'll do 64 */
static char* layer_directories[64];
static int layer_directory_count;

static const char* vanilla_layers[] =
{
	"luna.layer",
	"sta2_done.layer",
	"sta1_done.layer",
	"blank.layer",
	"surface.layer",
	"caverns.layer",
	"forest.layer",
	"ruins.layer",
	"whispy.layer",
	"city.layer",
	"treasuretemple.layer",
	"dino_den.layer",
	"magma.layer",
	"core.layer",
};

static struct layer
{
	char directory[MAX_PATH];
	sprite_t sprite;
} layer_list[COUNT_OF(vanilla_layers)];

static void viewer_reload_sprite(void)
{
	char buf[MAX_PATH + 27];
	if (mode == MODE_SCROLL_LAYERS)
	{
		int scroll = TARGET_HEIGHT - index % TARGET_HEIGHT;
		int layer_index = index / TARGET_HEIGHT;
		if (scroll < TARGET_HEIGHT / 2)
		{
			layer_index++;
		}
		snprintf(buf, sizeof buf, "\"%s\" @ %i - Scrolling base game layers", vanilla_layers[layer_index], index);
		screen_change_title(buf);
		screen_change_dirt_color(screen_sprite_dirt_color(layer_list[layer_index].sprite));
		screen_repaint();
		return;
	}

	char** directories = mode == MODE_VIEW_SPRITES ? sprite_directories : layer_directories;
	sprite_t next = file_load_sprite(directories[index]);
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
	screen_clear();
	screen_change_dirt_color(screen_sprite_dirt_color(current));
	screen_repaint();
}

void viewer_handle_repaint()
{
	if (mode == MODE_SCROLL_LAYERS)
	{
		int top = index / TARGET_HEIGHT;
		int bottom = index / TARGET_HEIGHT + 1;
		screen_sprite_render(0, -index % TARGET_HEIGHT, layer_list[top].sprite);
		screen_sprite_render(0, TARGET_HEIGHT - index % TARGET_HEIGHT, layer_list[bottom].sprite);
		return;
	}
	screen_sprite_render(TARGET_WIDTH / 2 - screen_sprite_width(current) / 2, TARGET_HEIGHT / 2 - screen_sprite_height(current) / 2, current);
}

void viewer_handle_keyboard(virtual_key_t vk)
{
	assert(COUNT_OF(vanilla_layers) > 1);
	int dir_count = mode == MODE_VIEW_SPRITES ? sprite_directory_count : 
		(mode == MODE_VIEW_LAYERS ? layer_directory_count : (TARGET_HEIGHT * (COUNT_OF(vanilla_layers) - 1)));
	switch (vk)
	{
	case VK_UP:
		if (mode == MODE_SCROLL_LAYERS)
		{
			index = max(index - scroll_speed, 0);
		}
		else
		{
			index = (index + dir_count - 1) % dir_count;
		}
		viewer_reload_sprite();
		break;
	case VK_DOWN:
		if (mode == MODE_SCROLL_LAYERS)
		{
			index = min(index + scroll_speed, dir_count - 1);
		}
		else
		{
			index = (index + 1) % dir_count;
		}
		viewer_reload_sprite();
		break;
	case VK_LEFT:
		mode = ((mode + MODE_COUNT) - 1) % MODE_COUNT;
		index = 0;
		viewer_reload_sprite();
		break;
	case VK_RIGHT:
		mode = (mode + 1) % MODE_COUNT;
		index = 0;
		viewer_reload_sprite();
		break;
	}
}

void viewer_handle_mouse_button(int x, int y)
{
	if (mode == MODE_SCROLL_LAYERS)
	{
		debug_format("%i, %i\n", x, y + index);
		return;
	}

	int sprite_x = TARGET_WIDTH / 2 - screen_sprite_width(current) / 2;
	int sprite_y = TARGET_HEIGHT / 2 - screen_sprite_height(current) / 2;
	if (x >= sprite_x && y >= sprite_y && x < sprite_x + screen_sprite_width(current) && y < sprite_y + screen_sprite_height(current))
	{
		debug_format("%i, %i\n", x - sprite_x, y - sprite_y);
	}
}

void viewer_handle_mouse_wheel(int direction)
{
	scroll_speed += direction;
	scroll_speed = min(max(scroll_speed, 1), 15);
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
	debug_format("Initializing sprite directories...\n");
	sprite_directory_count = viewer_initialize_directories(DIG_N_RIG_SPRITE_PATH, sprite_directories, sizeof sprite_directories / sizeof * sprite_directories);
	layer_directory_count = viewer_initialize_directories(DIG_N_RIG_LAYER_PATH, layer_directories, sizeof layer_directories / sizeof * layer_directories);

	debug_format("Loading base game's layers...\n");
	for (int i = 0; i < COUNT_OF(layer_list); i++)
	{
		snprintf(layer_list[i].directory, sizeof layer_list[i].directory, "%s%s", DIG_N_RIG_LAYER_PATH, vanilla_layers[i]);
		layer_list[i].sprite = file_load_sprite(layer_list[i].directory);
		if (!layer_list[i].sprite)
		{
			exit(-1);
			return;
		}
	}

	viewer_reload_sprite();
}

static void viewer_destroy(void)
{
	screen_sprite_destroy(current);
	for (int i = 0; i < sizeof sprite_directories / sizeof * sprite_directories; i++)
	{
		free(sprite_directories[i]);
	}
	for (int i = 0; i < sizeof layer_directories / sizeof * layer_directories; i++)
	{
		free(layer_directories[i]);
	}
	for (int i = 0; i < COUNT_OF(layer_list); i++)
	{
		screen_sprite_destroy(layer_list[i].sprite);
	}
}

int main()
{
	screen_initialize((screen_events_t) { viewer_handle_repaint, viewer_handle_keyboard, viewer_handle_mouse_button, viewer_handle_mouse_wheel });
	viewer_initialize();
	
	screen_loop();

	screen_destroy();

	return 0;
}