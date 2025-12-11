/*
	screen.c ~ RL
*/

#include "screen.h"

#include "debug.h"
#include "file.h"
#include "types.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>

#define SCREEN_FONT L"digfont9"
#define RUNTIME_ASSERT(cond) if (!(cond)) exit(-1);

#define RAISE_EVENT(ev, ...) if (ev) ev(__VA_ARGS__);

struct sprite
{
	int width, height;
	uint32_t palette_id;
	CHAR_INFO* data;
};

static HANDLE in, out;
static screen_events_t events;
static CHAR_INFO blank[TARGET_WIDTH * TARGET_HEIGHT];

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
			if (ker.wVirtualKeyCode == VK_ESCAPE)
			{
				break;
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
		}
	}
}

void screen_repaint(void)
{
	screen_clear();
	RAISE_EVENT(events.repaint);
}

void screen_clear(void)
{
	SMALL_RECT window_size = { .Top = 0, .Left = 0, .Right = TARGET_WIDTH, .Bottom = TARGET_HEIGHT };
	WriteConsoleOutputW(out, blank, (COORD) { TARGET_WIDTH, TARGET_HEIGHT }, (COORD) { 0, 0 }, & window_size);
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

sprite_t screen_sprite_create(int width, int height, uint32_t palette_id, char* text, attribute_t* attrib)
{
	RUNTIME_ASSERT(text && attrib);
	sprite_t res = dig_malloc(sizeof * res);
	res->width = width;
	res->height = height;
	res->palette_id = palette_id;
	res->data = dig_malloc(width * height * sizeof * res->data);
	for (int i = 0; i < width * height; i++)
	{
		res->data[i].Char.AsciiChar = text[i];
		res->data[i].Attributes = attrib[i];
	}
	return res;
}

void screen_sprite_destroy(sprite_t sprite)
{
	if (!sprite)
	{
		return;
	}
	free(sprite->data);
	free(sprite);
}

void screen_sprite_render(int x, int y, sprite_t sprite)
{
	RUNTIME_ASSERT(sprite);
	SMALL_RECT write_region = { .Left = x, .Top = y, .Right = x + sprite->width, .Bottom = y + sprite->height };
	WriteConsoleOutputA(out, sprite->data, (COORD) { sprite->width, sprite->height }, (COORD) { 0, 0 }, & write_region);
}

int screen_sprite_width(sprite_t sprite)
{
	RUNTIME_ASSERT(sprite);
	return sprite->width;
}

int screen_sprite_height(sprite_t sprite)
{
	RUNTIME_ASSERT(sprite);
	return sprite->height;
}

uint32_t screen_sprite_palette_id(sprite_t sprite)
{
	RUNTIME_ASSERT(sprite);
	return sprite->palette_id;
}