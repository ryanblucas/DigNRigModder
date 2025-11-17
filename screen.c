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

struct sprite
{
	int width, height;
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
	csbi.srWindow.Right = csbi.dwSize.X - 1;
	csbi.srWindow.Bottom = csbi.dwSize.Y - 1;

	for (int i = 0; i < 16; i++)
	{
		int intensity = !!(i & 8) * 128 + 127;
		csbi.ColorTable[i] = RGB(!!(i & 4) * intensity, !!(i & 2) * intensity, !!(i & 1) * intensity);
	}
	csbi.ColorTable[LIGHT_GRAY] = RGB(192, 192, 192);
	csbi.ColorTable[DARK_GRAY] = RGB(128, 128, 128);

	RUNTIME_ASSERT(SetConsoleScreenBufferInfoEx(out, &csbi));
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

	RUNTIME_ASSERT(SetConsoleMode(in, ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT));
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
			else if (ker.wVirtualKeyCode == VK_LEFT)
			{
				events.left();
			}
			else if (ker.wVirtualKeyCode == VK_RIGHT)
			{
				events.right();
			}
		}
		else if (ir.EventType == WINDOW_BUFFER_SIZE_EVENT)
		{
			HWND console_window = GetConsoleWindow();

			RECT fitted = (RECT){ .right = TARGET_WIDTH * TARGET_CELL_SIZE, .bottom = TARGET_HEIGHT * TARGET_CELL_SIZE };
			RUNTIME_ASSERT(AdjustWindowRectEx(&fitted, GetWindowLongW(console_window, GWL_STYLE), FALSE, GetWindowLongW(console_window, GWL_EXSTYLE)));

			RUNTIME_ASSERT(SetWindowPos(console_window, NULL, 0, 0, fitted.right - fitted.left, fitted.bottom - fitted.top, SWP_NOMOVE));
			screen_initialize_cursor();
			screen_repaint();

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
	SMALL_RECT window_size = { .Top = 0, .Left = 0, .Right = TARGET_WIDTH, .Bottom = TARGET_HEIGHT };
	WriteConsoleOutputW(out, blank, (COORD) { TARGET_WIDTH, TARGET_HEIGHT }, (COORD) { 0, 0 }, & window_size);
	events.repaint();
}

void screen_change_title(const char* title)
{
	RUNTIME_ASSERT(SetConsoleTitleA(title));
}

sprite_t screen_sprite_create(int width, int height, char* text, attribute_t* attrib)
{
	RUNTIME_ASSERT(text && attrib);
	sprite_t res = dig_malloc(sizeof * res);
	res->width = width;
	res->height = height;
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
	RUNTIME_ASSERT(WriteConsoleOutputA(out, sprite->data, (COORD) { sprite->width, sprite->height }, (COORD) { 0, 0 }, &write_region));
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