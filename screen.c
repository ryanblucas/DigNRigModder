/*
	screen.c ~ RL
	Handles console screen
*/

#include "screen.h"

#include "debug.h"
#include "file.h"
#include "types.h"
#include <stdalign.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>

#define SCREEN_FONT L"digfont9"

#define RAISE_EVENT(ev, ...) if (ev) ev(__VA_ARGS__);

struct sprite
{
	int width, height;
	uint32_t dirt_color;
	CHAR_INFO data[];
};

static HANDLE in, out;
static screen_events_t events;
static CHAR_INFO target[TARGET_WIDTH * TARGET_HEIGHT];

static void screen_initialize_output()
{
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
}

static void screen_initialize_cursor(void)
{
	CONSOLE_CURSOR_INFO cci;
	RUNTIME_ASSERT(GetConsoleCursorInfo(out, &cci));
	cci.bVisible = FALSE;
	RUNTIME_ASSERT(SetConsoleCursorInfo(out, &cci));
}

void screen_initialize(screen_events_t _events)
{
	in = GetStdHandle(STD_INPUT_HANDLE);
	RUNTIME_ASSERT(in != INVALID_HANDLE_VALUE && in);

	out = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	RUNTIME_ASSERT(out != INVALID_HANDLE_VALUE);

	screen_initialize_output();

	CONSOLE_FONT_INFOEX cfi = { .cbSize = sizeof cfi };
	RUNTIME_ASSERT(GetCurrentConsoleFontEx(out, FALSE, &cfi));

	cfi.dwFontSize = (COORD){ TARGET_CELL_SIZE - 1, TARGET_CELL_SIZE };
	cfi.FontFamily = FF_DONTCARE;
	cfi.nFont = 0;
	swprintf(cfi.FaceName, sizeof cfi.FaceName / sizeof * cfi.FaceName, SCREEN_FONT);

	RUNTIME_ASSERT(SetCurrentConsoleFontEx(out, FALSE, &cfi));

	RUNTIME_ASSERT(SetConsoleActiveScreenBuffer(out));

	RUNTIME_ASSERT(SetConsoleMode(in, ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT));
	RUNTIME_ASSERT(SetConsoleMode(out, 0));
	
	screen_initialize_cursor();
	events = _events;

	RUNTIME_ASSERT(GetCurrentConsoleFontEx(out, FALSE, &cfi));

	if (wcsncmp(cfi.FaceName, SCREEN_FONT, sizeof cfi.FaceName / sizeof * cfi.FaceName) != 0)
	{
		debug_format("Failed to locate Dig-N-Rig's font!\n");
	}
}

void screen_destroy(void)
{
	CloseHandle(out);
}

void screen_loop(void)
{
	INPUT_RECORD ir;
	DWORD read;
	while (ReadConsoleInputW(in, &ir, 1, &read) && read == 1)
	{
		if (ir.EventType == KEY_EVENT)
		{
			KEY_EVENT_RECORD ker = ir.Event.KeyEvent;
			if (!ker.bKeyDown)
			{
				continue;
			}
			RAISE_EVENT(events.keyboard, ker.wVirtualKeyCode);
		}
		else if (ir.EventType == MOUSE_EVENT)
		{
			MOUSE_EVENT_RECORD mer = ir.Event.MouseEvent;
			if (mer.dwEventFlags == MOUSE_WHEELED)
			{
				WORD scroll = HIWORD(mer.dwButtonState);
				RAISE_EVENT(events.mouse_wheel, (signed short)scroll / WHEEL_DELTA);
			}
			else if (mer.dwEventFlags == 0) /* release or click */
			{
				static DWORD previous_button_state = 0;
				if (mer.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED && previous_button_state ^ FROM_LEFT_1ST_BUTTON_PRESSED)
				{
					RAISE_EVENT(events.mouse_button, mer.dwMousePosition.X, mer.dwMousePosition.Y);
				}
			}
		}
		else if (ir.EventType == WINDOW_BUFFER_SIZE_EVENT)
		{
			HWND console_window = GetConsoleWindow();

			RECT to_compare;
			GetWindowRect(console_window, &to_compare);

			RECT fitted = (RECT){ .right = TARGET_WIDTH * TARGET_CELL_SIZE, .bottom = TARGET_HEIGHT * TARGET_CELL_SIZE };
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
}

void screen_repaint(void)
{
	RAISE_EVENT(events.repaint);
	screen_invalidate();
}

void screen_invalidate(void)
{
	SMALL_RECT window_size = { .Top = 0, .Left = 0, .Right = TARGET_WIDTH, .Bottom = TARGET_HEIGHT };
	WriteConsoleOutputA(out, target, (COORD) { TARGET_WIDTH, TARGET_HEIGHT }, (COORD) { 0, 0 }, & window_size);
}

void screen_clear(void)
{
	memset(target, 0, sizeof target);
}

void screen_change_title(const char* title)
{
	RUNTIME_ASSERT(SetConsoleTitleA(title));
}

void screen_change_dirt_color(uint32_t rgb)
{
	CONSOLE_SCREEN_BUFFER_INFOEX csbi = { .cbSize = sizeof csbi };
	RUNTIME_ASSERT(GetConsoleScreenBufferInfoEx(out, &csbi));

	csbi.ColorTable[0] = RGB(0, 0, 0);
	csbi.ColorTable[1] = RGB(67, 52, 172);
	csbi.ColorTable[2] = RGB(44, 109, 67);
	csbi.ColorTable[3] = RGB(45, 97, 143);
	csbi.ColorTable[4] = RGB(129, 14, 44);
	csbi.ColorTable[5] = RGB(97, 32, 121);
	csbi.ColorTable[6] = rgb;
	csbi.ColorTable[7] = RGB(161, 159, 159);
	csbi.ColorTable[8] = RGB(97, 95, 115);
	csbi.ColorTable[9] = RGB(78, 131, 255);
	csbi.ColorTable[10] = RGB(155, 230, 91);
	csbi.ColorTable[11] = RGB(132, 205, 241);
	csbi.ColorTable[12] = RGB(235, 40, 57);
	csbi.ColorTable[13] = RGB(221, 140, 239);
	csbi.ColorTable[14] = RGB(252, 236, 84);
	csbi.ColorTable[15] = RGB(232, 232, 238);

	/* why is this needed? */
	csbi.srWindow.Bottom = csbi.dwSize.Y;

	RUNTIME_ASSERT(SetConsoleScreenBufferInfoEx(out, &csbi));
}

/* these functions are essentially the same logic, but it's more expensive to put it all in one function since they will all need to adapt to it */

static inline void screen_buffer_set_char_region(CHAR_INFO* buffer, int bwx, int bwy, const char* in, int x, int y, int wx, int wy)
{
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

static inline void screen_buffer_set_attrib_region(CHAR_INFO* buffer, int bwx, int bwy, const attribute_t* in, int x, int y, int wx, int wy)
{
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

void screen_set_char_region(const char* in, int x, int y, int wx, int wy)
{
	screen_buffer_set_char_region(target, TARGET_WIDTH, TARGET_HEIGHT, in, x, y, wx, wy);
}

void screen_set_attrib_region(const attribute_t* in, int x, int y, int wx, int wy)
{
	screen_buffer_set_attrib_region(target, TARGET_WIDTH, TARGET_HEIGHT, in, x, y, wx, wy);
}

static inline int screen_buffer_get_char_region(const CHAR_INFO* buffer, int bwx, int bwy, char* out, int x, int y, int wx, int wy)
{
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

static inline int screen_buffer_get_attrib_region(const CHAR_INFO* buffer, int bwx, int bwy, attribute_t* out, int x, int y, int wx, int wy)
{
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

int screen_get_char_region(char* out, int x, int y, int wx, int wy)
{
	return screen_buffer_get_char_region(target, TARGET_WIDTH, TARGET_HEIGHT, out, x, y, wx, wy);
}

int screen_get_attrib_region(attribute_t* out, int x, int y, int wx, int wy)
{
	return screen_buffer_get_attrib_region(target, TARGET_WIDTH, TARGET_HEIGHT, out, x, y, wx, wy);
}

sprite_t screen_sprite_create(int width, int height, uint32_t dirt_color, char* text, attribute_t* attrib)
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
	if (right < left || bottom < top)
	{
		return;
	}
	for (int cy = top; cy < bottom; cy++)
	{
		memcpy(target + (left + cy * TARGET_WIDTH), sprite->data + (cy - y) * sprite->width, (right - left) * sizeof * sprite->data);
	}
}

void screen_sprite_set_char_region(sprite_t sprite, const char* in, int x, int y, int wx, int wy)
{
	screen_buffer_set_char_region(sprite->data, sprite->width, sprite->height, in, x, y, wx, wy);
}

void screen_sprite_set_attrib_region(sprite_t sprite, const attribute_t* in, int x, int y, int wx, int wy)
{
	screen_buffer_set_attrib_region(sprite->data, sprite->width, sprite->height, in, x, y, wx, wy);
}

int screen_sprite_get_char_region(const sprite_t sprite, char* out, int x, int y, int wx, int wy)
{
	return screen_buffer_get_char_region(sprite->data, sprite->width, sprite->height, out, x, y, wx, wy);
}

int screen_sprite_get_attrib_region(const sprite_t sprite, attribute_t* out, int x, int y, int wx, int wy)
{
	return screen_buffer_get_attrib_region(sprite->data, sprite->width, sprite->height, out, x, y, wx, wy);
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