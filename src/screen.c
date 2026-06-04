/*
	screen.c ~ RL
	Handles console screen
*/

#include "screen.h"

#include "debug.h"
#include "event_queue.h"
#include "file.h"
#include "types.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>

struct sprite
{
	int width, height;
	rgb_color_t dirt_color;
	CHAR_INFO data[];
};

static HANDLE in, out;
static screen_events_t events;
static CHAR_INFO buffer[3 * TARGET_WIDTH * TARGET_HEIGHT];
static CHAR_INFO* target = buffer;

static int cell_size = TARGET_CELL_SIZE;

static bool used_period;
static volatile LONG flag_running;

const rgb_color_t palette[16] =
{
	RGB(0, 0, 0),
	RGB(67, 52, 172),
	RGB(44, 109, 67),
	RGB(45, 97, 143),
	RGB(129, 14, 44),
	RGB(97, 32, 121),
	DNR_DEFAULT_DIRT_COLOR,
	RGB(161, 159, 159),
	RGB(97, 95, 115),
	RGB(78, 131, 255),
	RGB(155, 230, 91),
	RGB(132, 205, 241),
	RGB(235, 40, 57),
	RGB(221, 140, 239),
	RGB(252, 236, 84),
	RGB(232, 232, 238),
};

void screen_set_event_handlers(const screen_events_t* _events)
{
	static screen_simulator_t empty_simulators[] = { NULL };
	if (events.simulators && events.simulators != empty_simulators)
	{
		free(events.simulators);
	}
	events = *_events;
	if (!events.simulators || !*events.simulators)
	{
		events.simulators = empty_simulators;
		return;
	}
	int count = 0;
	for (screen_simulator_t* sim = events.simulators; sim && *sim; sim++)
	{
		count++;
	}
	events.simulators = dig_malloc((count + 1) * sizeof * events.simulators);
	memcpy(events.simulators, _events->simulators, (count + 1) * sizeof * events.simulators);
}

static void screen_initialize_input(void)
{
	in = GetStdHandle(STD_INPUT_HANDLE);
	RUNTIME_ASSERT(in != INVALID_HANDLE_VALUE && in);
	RUNTIME_ASSERT(SetConsoleMode(in, ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT));
}

static void screen_initialize_output(void)
{
	out = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	RUNTIME_ASSERT(out != INVALID_HANDLE_VALUE);

	CONSOLE_SCREEN_BUFFER_INFOEX csbi = { .cbSize = sizeof csbi };
	RUNTIME_ASSERT(GetConsoleScreenBufferInfoEx(out, &csbi));

	csbi.dwMaximumWindowSize.X = TARGET_WIDTH;
	csbi.dwMaximumWindowSize.Y = TARGET_HEIGHT;
	csbi.dwSize = csbi.dwMaximumWindowSize;

	csbi.srWindow.Left = 0;
	csbi.srWindow.Top = 0;
	csbi.srWindow.Right = csbi.dwSize.X;
	csbi.srWindow.Bottom = csbi.dwSize.Y;

	RUNTIME_ASSERT(SetConsoleScreenBufferInfoEx(out, &csbi));

	screen_change_dirt_color(1);

	RUNTIME_ASSERT(SetConsoleMode(out, 0));
}

static void screen_initialize_font(bool is_small_console)
{
	WCHAR* font_name = is_small_console ? DNR_FONT_SMALL : DNR_FONT;

	CONSOLE_FONT_INFOEX cfi = { .cbSize = sizeof cfi };
	RUNTIME_ASSERT(GetCurrentConsoleFontEx(out, FALSE, &cfi));
	cell_size = is_small_console ? TARGET_CELL_SIZE_SMALL : TARGET_CELL_SIZE;
	cfi.dwFontSize = (COORD){ cell_size - 1, cell_size };
	cfi.FontFamily = FF_DONTCARE;
	cfi.nFont = 0;
	swprintf(cfi.FaceName, sizeof cfi.FaceName / sizeof * cfi.FaceName, font_name);
	RUNTIME_ASSERT(SetCurrentConsoleFontEx(out, FALSE, &cfi));

	RUNTIME_ASSERT(GetCurrentConsoleFontEx(out, FALSE, &cfi));
	if (wcsncmp(cfi.FaceName, font_name, sizeof cfi.FaceName / sizeof * cfi.FaceName) != 0)
	{
		debug_format("Failed to locate Dig-N-Rig's font!\n");
	}
}

static void screen_initialize_cursor(void)
{
	CONSOLE_CURSOR_INFO cci;
	RUNTIME_ASSERT(GetConsoleCursorInfo(out, &cci));
	cci.bVisible = FALSE;
	RUNTIME_ASSERT(SetConsoleCursorInfo(out, &cci));
}

void screen_initialize(bool is_small_console)
{
	screen_initialize_input();
	screen_initialize_output();
	screen_initialize_font(is_small_console);
	RUNTIME_ASSERT(SetConsoleActiveScreenBuffer(out));
	screen_initialize_cursor();

	used_period = timeBeginPeriod(1) == TIMERR_NOERROR;
	if (!used_period)
	{
		debug_format("Failed to set timer resolution, simulators may not run at specified rate.\n");
	}
}

void screen_destroy(void)
{
	CloseHandle(out);
	free(events.simulators);
	out = NULL;
	if (used_period)
	{
		timeEndPeriod(1);
	}
}

/* that last condition is scary b/c it works on my machine, but it isn't guaranteed.
   Hence, why this is a macro cause I wanna be able to fix this condition */
#define IS_FOCUS_RECORD(ir) ((ir).EventType == FOCUS_EVENT && (ir).Event.FocusEvent.bSetFocus)

static void screen_handle_input(const INPUT_RECORD* ir, DWORD* prev_button_state)
{
	if (ir->EventType == KEY_EVENT)
	{
		KEY_EVENT_RECORD ker = ir->Event.KeyEvent;
		if (!ker.bKeyDown)
		{
			return;
		}
		RAISE_EVENT(events.keyboard, ker.wVirtualKeyCode, ker.dwControlKeyState);
	}
	else if (ir->EventType == MOUSE_EVENT)
	{
		MOUSE_EVENT_RECORD mer = ir->Event.MouseEvent;
		if (mer.dwEventFlags == MOUSE_WHEELED)
		{
			WORD scroll = HIWORD(mer.dwButtonState);
			RAISE_EVENT(events.mouse_wheel, (signed short)scroll / WHEEL_DELTA);
		}
		else if (mer.dwEventFlags == 0 && (mer.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED || *prev_button_state & FROM_LEFT_1ST_BUTTON_PRESSED))
		{
			RAISE_EVENT(events.mouse_button, mer.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED, mer.dwMousePosition.X, mer.dwMousePosition.Y);
		}
		else if (mer.dwEventFlags == MOUSE_MOVED)
		{
			RAISE_EVENT(events.mouse_move, mer.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED, mer.dwMousePosition.X, mer.dwMousePosition.Y);
		}
		*prev_button_state = mer.dwButtonState;
	}
	else if (IS_FOCUS_RECORD(*ir))
	{
		POINT pt;
		RUNTIME_ASSERT(GetCursorPos(&pt));
		RUNTIME_ASSERT(ScreenToClient(GetConsoleWindow(), &pt));
		pt.x /= cell_size;
		pt.y /= cell_size;
		RAISE_EVENT(events.mouse_button, true, pt.x, pt.y);
		*prev_button_state = FROM_LEFT_1ST_BUTTON_PRESSED;
	}
	else if (ir->EventType == WINDOW_BUFFER_SIZE_EVENT)
	{
		HWND console_window = GetConsoleWindow();

		RECT to_compare;
		GetWindowRect(console_window, &to_compare);

		RECT fitted = (RECT){ .right = TARGET_WIDTH * cell_size, .bottom = TARGET_HEIGHT * cell_size };
		RUNTIME_ASSERT(AdjustWindowRectEx(&fitted, GetWindowLongW(console_window, GWL_STYLE), FALSE, GetWindowLongW(console_window, GWL_EXSTYLE)));

		RUNTIME_ASSERT(SetWindowPos(console_window, NULL, 0, 0, fitted.right - fitted.left, fitted.bottom - fitted.top, SWP_NOMOVE));
		screen_initialize_cursor();
		RAISE_EVENT(events.repaint);

		CONSOLE_SCREEN_BUFFER_INFOEX csbi = { .cbSize = sizeof csbi };
		RUNTIME_ASSERT(GetConsoleScreenBufferInfoEx(out, &csbi));
		if (csbi.dwSize.X != TARGET_WIDTH || csbi.dwSize.Y != TARGET_HEIGHT)
		{
			screen_initialize_output();
		}
		screen_invalidate();
	}
}

static void screen_simulator_run(LARGE_INTEGER start, LARGE_INTEGER last, LARGE_INTEGER frequency)
{
	target = buffer + TARGET_WIDTH * TARGET_HEIGHT;
	memset(target, 0, TARGET_WIDTH * TARGET_HEIGHT * sizeof(CHAR_INFO));

	float delta = (float)(start.QuadPart - last.QuadPart) / frequency.QuadPart;
	screen_simulator_t* sim = events.simulators;
	while (*sim)
	{
		(*sim)(delta);
		sim++;
	}

	target = buffer;
	screen_invalidate();
}

static void screen_loop_no_simulation(void)
{
	debug_format("Running screen loop with no simulators\n");
	INPUT_RECORD ir;
	DWORD read;
	bool consumed_first_focus = false;
	DWORD prev_button_state = 0;
	InterlockedExchange(&flag_running, TRUE);
	while (ReadConsoleInputW(in, &ir, 1, &read) && read == 1 && GetConsoleWindow() != NULL)
	{
		queue_run();
		if (IS_FOCUS_RECORD(ir) && !consumed_first_focus)
		{
			consumed_first_focus = true;
			continue;
		}
		screen_handle_input(&ir, &prev_button_state);
	}
	InterlockedExchange(&flag_running, FALSE);
}

void screen_loop(int events_per_frame, int simulation_framerate)
{
	if (simulation_framerate <= 0)
	{
		screen_loop_no_simulation();
		return;
	}

	LARGE_INTEGER frequency, last;
	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&last);

	INPUT_RECORD ir;
	DWORD read;
	bool consumed_first_focus = false;
	DWORD prev_button_state = 0;
	InterlockedExchange(&flag_running, TRUE);
	while (GetConsoleWindow() != NULL)
	{
		LARGE_INTEGER start, end;
		QueryPerformanceCounter(&start);

		/* != so that if events_per_frame is negative, it means it will process every event adding more functionality to the property */
		for (int i = 0; i != events_per_frame && PeekConsoleInputW(in, &ir, 1, &read) && read == 1; i++)
		{
			ReadConsoleInputW(in, &ir, 1, &read);
			if (IS_FOCUS_RECORD(ir) && !consumed_first_focus)
			{
				consumed_first_focus = true;
				continue;
			}
			screen_handle_input(&ir, &prev_button_state);
		}

		screen_simulator_run(start, last, frequency);
		queue_run();

		QueryPerformanceCounter(&end);
		DWORD ms = (DWORD)((end.QuadPart - start.QuadPart) * 1000 / frequency.QuadPart);
		int desired_frame_time = 1000 / simulation_framerate;
		if ((int)ms < desired_frame_time)
		{
			Sleep(desired_frame_time - ms);
		}
		last = start;
	}
	InterlockedExchange(&flag_running, FALSE);
}

void screen_repaint(void)
{
	RAISE_EVENT(events.repaint);
	screen_invalidate();
}

void screen_invalidate(void)
{
	CHAR_INFO* cache_layer = buffer;
	CHAR_INFO* sim_layer = buffer + TARGET_WIDTH * TARGET_HEIGHT;
	CHAR_INFO* combined_layer = buffer + TARGET_WIDTH * TARGET_HEIGHT * 2;
	for (int i = 0; i < TARGET_WIDTH * TARGET_HEIGHT; i++)
	{
		combined_layer[i] = sim_layer[i].Char.AsciiChar == 0 && sim_layer[i].Attributes == 0 ? cache_layer[i] : sim_layer[i];
	}
	SMALL_RECT window_size = { .Top = 0, .Left = 0, .Right = TARGET_WIDTH, .Bottom = TARGET_HEIGHT };
	WriteConsoleOutputA(out, combined_layer, (COORD) { TARGET_WIDTH, TARGET_HEIGHT }, (COORD) { 0, 0 }, & window_size);
}

void screen_clear(void)
{
	memset(target, 0, TARGET_WIDTH * TARGET_HEIGHT * sizeof(CHAR_INFO));
}

rgb_color_t screen_dirt_color(void)
{
	CONSOLE_SCREEN_BUFFER_INFOEX csbi = { .cbSize = sizeof csbi };
	RUNTIME_ASSERT(GetConsoleScreenBufferInfoEx(out, &csbi));
	return csbi.ColorTable[6];
}

void screen_change_title(const char* title)
{
	RUNTIME_ASSERT(SetConsoleTitleA(title));
}

void screen_change_dirt_color(rgb_color_t rgb)
{
	CONSOLE_SCREEN_BUFFER_INFOEX csbi = { .cbSize = sizeof csbi };
	RUNTIME_ASSERT(GetConsoleScreenBufferInfoEx(out, &csbi));

	for (int i = 0; i < 16; i++)
	{
		csbi.ColorTable[i] = palette[i];
	}

	csbi.ColorTable[DNR_DIRT_INDEX] = rgb;

	/* why is this needed? */
	csbi.srWindow.Bottom = csbi.dwSize.Y;

	RUNTIME_ASSERT(SetConsoleScreenBufferInfoEx(out, &csbi));
}

/* these functions are essentially the same logic, but it's more expensive to put it all in one function */

static inline void screen_buffer_set_char_region(CHAR_INFO* buffer, int bwx, int bwy, const char* in, region_t region)
{
	int x = region.x0, y = region.y0, wx = region.x1 - region.x0 + 1, wy = region.y1 - region.y0 + 1;

	int top = max(y, 0),
		bottom = min(y + wy, bwy);
	int left = max(x, 0),
		right = min(x + wx, bwx);
	if (right < left || bottom < top)
	{
		return;
	}
	for (int cy = top; cy < bottom; cy++)
	{
		/* test if SIMD is faster */
		for (int i = 0; i < right - left; i++)
		{
			(buffer + (left + cy * bwx) + i)->Char.AsciiChar = *(in + (cy - y) * wx + i);
		}
	}
}

static inline void screen_buffer_set_attrib_region(CHAR_INFO* buffer, int bwx, int bwy, const attribute_t* in, region_t region)
{
	int x = region.x0, y = region.y0, wx = region.x1 - region.x0 + 1, wy = region.y1 - region.y0 + 1;

	int top = max(y, 0),
		bottom = min(y + wy, bwy);
	int left = max(x, 0),
		right = min(x + wx, bwx);
	if (right < left || bottom < top)
	{
		return;
	}
	for (int cy = top; cy < bottom; cy++)
	{
		/* test if SIMD is faster */
		for (int i = 0; i < right - left; i++)
		{
			(buffer + (left + cy * bwx) + i)->Attributes = *(in + (cy - y) * wx + i);
		}
	}
}

void screen_set_char_region(const char* in, region_t region)
{
	screen_buffer_set_char_region(target, TARGET_WIDTH, TARGET_HEIGHT, in, region);
}

void screen_set_attrib_region(const attribute_t* in, region_t region)
{
	screen_buffer_set_attrib_region(target, TARGET_WIDTH, TARGET_HEIGHT, in, region);
}

static inline int screen_buffer_get_char_region(const CHAR_INFO* buffer, int bwx, int bwy, char* out, region_t region)
{
	int x = region.x0, y = region.y0, wx = region.x1 - region.x0 + 1, wy = region.y1 - region.y0 + 1;

	if (wx <= 0 || wy <= 0)
	{
		return 0;
	}

	memset(out, 0, wx * wy * sizeof * out);
	int top = max(y, 0),
		bottom = min(y + wy, bwy);
	int left = max(x, 0),
		right = min(x + wx, bwx);
	if (right < left || bottom < top)
	{
		return 0;
	}
	for (int cy = top; cy < bottom; cy++)
	{
		/* test if SIMD is faster */
		for (int i = 0; i < right - left; i++)
		{
			*(out + (cy - y) * wx + i) = (buffer + (left + cy * bwx) + i)->Char.AsciiChar;
		}
	}
	return (bottom - top) * (right - left);
}

static inline int screen_buffer_get_attrib_region(const CHAR_INFO* buffer, int bwx, int bwy, attribute_t* out, region_t region)
{
	int x = region.x0, y = region.y0, wx = region.x1 - region.x0 + 1, wy = region.y1 - region.y0 + 1;

	if (wx <= 0 || wy <= 0)
	{
		return 0;
	}

	memset(out, 0, wx * wy * sizeof * out);
	int top = max(y, 0),
		bottom = min(y + wy, bwy);
	int left = max(x, 0),
		right = min(x + wx, bwx);
	if (right < left || bottom < top)
	{
		return 0;
	}
	for (int cy = top; cy < bottom; cy++)
	{
		/* test if SIMD is faster */
		for (int i = 0; i < right - left; i++)
		{
			*(out + (cy - y) * wx + i) = (buffer + (left + cy * bwx) + i)->Attributes;
		}
	}
	return (bottom - top) * (right - left);
}

int screen_get_char_region(char* out, region_t region)
{
	return screen_buffer_get_char_region(target, TARGET_WIDTH, TARGET_HEIGHT, out, region);
}

int screen_get_attrib_region(attribute_t* out, region_t region)
{
	return screen_buffer_get_attrib_region(target, TARGET_WIDTH, TARGET_HEIGHT, out, region);
}

void screen_invert_region(region_t region)
{
	if (region_is_invalid(region))
	{
		return;
	}
	region = region_keep_inside(region, (region_t) { 0, 0, WORLD_WIDTH - 1, WORLD_HEIGHT - 1 });
	attribute_t* selected = dig_malloc(region_size(region) * sizeof * selected);
	screen_get_attrib_region(selected, region);
	for (int i = 0; i < region_size(region); i++)
	{
		selected[i] = ~selected[i] & 0xFF;
	}
	screen_set_attrib_region(selected, region);
	free(selected);
}

sprite_t screen_sprite_create(int width, int height, rgb_color_t dirt_color, char* text, attribute_t* attrib)
{
	RUNTIME_ASSERT(text && attrib);
	sprite_t res = dig_malloc(sizeof * res + width * height * sizeof * res->data);
	res->width = width;
	res->height = height;
	res->dirt_color = dirt_color;
	for (int i = 0; i < width * height; i++)
	{
		res->data[i].Char.AsciiChar = text[i];
		res->data[i].Attributes = attrib[i];
	}
	return res;
}

void screen_sprite_destroy(sprite_t sprite)
{
	free(sprite);
}

void screen_sprite_render(int x, int y, const sprite_t sprite)
{
	RUNTIME_ASSERT(sprite);
	int top = max(y, 0),
		bottom = min(y + sprite->height, TARGET_HEIGHT);
	int left = max(x, 0),
		right = min(x + sprite->width, TARGET_WIDTH);
	if (right <= left || bottom <= top)
	{
		return;
	}
	for (int cy = top; cy < bottom; cy++)
	{
		memcpy(target + (left + cy * TARGET_WIDTH), sprite->data + (cy - y) * sprite->width - (x - left), (right - left) * sizeof * sprite->data);
	}
}

void screen_sprite_set_char_region(sprite_t sprite, const char* in, region_t region)
{
	screen_buffer_set_char_region(sprite->data, sprite->width, sprite->height, in, region);
}

void screen_sprite_set_attrib_region(sprite_t sprite, const attribute_t* in, region_t region)
{
	screen_buffer_set_attrib_region(sprite->data, sprite->width, sprite->height, in, region);
}

int screen_sprite_get_char_region(const sprite_t sprite, char* out, region_t region)
{
	return screen_buffer_get_char_region(sprite->data, sprite->width, sprite->height, out, region);
}

int screen_sprite_get_attrib_region(const sprite_t sprite, attribute_t* out, region_t region)
{
	return screen_buffer_get_attrib_region(sprite->data, sprite->width, sprite->height, out, region);
}

int screen_sprite_width(const sprite_t sprite)
{
	RUNTIME_ASSERT(sprite);
	return sprite->width;
}

int screen_sprite_height(const sprite_t sprite)
{
	RUNTIME_ASSERT(sprite);
	return sprite->height;
}

uint32_t screen_sprite_dirt_color(const sprite_t sprite)
{
	RUNTIME_ASSERT(sprite);
	return sprite->dirt_color;
}

void screen_sprite_set_dirt_color(sprite_t sprite, rgb_color_t dirt_color)
{
	RUNTIME_ASSERT(sprite);
	sprite->dirt_color = dirt_color;
}

void screen_wait_for_end(void)
{
	while (InterlockedOr(&flag_running, 0))
	{
		Sleep(0);
	}
}