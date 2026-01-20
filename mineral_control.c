/*
	mineral_control.c ~ RL
*/

#include "mineral_control.h"
#include "debug.h"
#include "screen.h"
#include <stdio.h>
#include "types.h"
#include <Windows.h>

struct mineral_control_internal
{
	char character;
	attribute_t attrib;
	COLORREF dirt_color;
	RECT rect;
	HFONT msg_font;
	char msg[2048];
};

static HFONT dnr_font;

static inline COLORREF mineral_control_get_color(struct mineral_control_internal* mci, int index)
{
	if (index == DNR_DIRT_INDEX)
	{
		return mci->dirt_color;
	}
	return palette[index];
}

static LRESULT mineral_control_window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	struct mineral_control_internal* mci = (struct mineral_control_internal*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	switch (msg)
	{
	case WM_CREATE:
	{
		CREATESTRUCT* cs = (CREATESTRUCT*)lparam;
		mci = dig_malloc(sizeof * mci);
		memset(mci, 0, sizeof * mci);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)mci);
		mci->rect.right = cs->cx;
		mci->rect.bottom = cs->cy;
		return 0;
	}
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		RUNTIME_ASSERT(BeginPaint(hwnd, &ps));

		Rectangle(ps.hdc, 0, 0, mci->rect.right, mci->rect.bottom);

		HDC memory_dc = CreateCompatibleDC(ps.hdc);
		HBITMAP memory_bmp = CreateCompatibleBitmap(ps.hdc, mci->rect.right, mci->rect.bottom);

		SelectObject(memory_dc, memory_bmp);

		SelectObject(memory_dc, dnr_font);
		SetTextColor(memory_dc, mineral_control_get_color(mci, ATTRIBUTE_FOREGROUND(mci->attrib)));
		SetBkColor(memory_dc, mineral_control_get_color(mci, ATTRIBUTE_BACKGROUND(mci->attrib)));
		TextOutA(memory_dc, 0, 0, &mci->character, 1);

		int size = (mci->rect.bottom - 8) / 8 * 8;
		StretchBlt(ps.hdc, 4, (mci->rect.bottom - size) / 2, size, size, memory_dc, 0, 0, 8, 8, SRCCOPY);

		DeleteObject(memory_bmp);
		DeleteObject(memory_dc);

		RECT region = { 0 };
		region.left = size + 4;
		region.right = mci->rect.right - 4;
		region.top = 4;
		region.bottom = mci->rect.bottom - 4;
		SelectObject(ps.hdc, mci->msg_font);
		DrawTextA(ps.hdc, mci->msg, -1, &region, DT_TOP | DT_LEFT);

		EndPaint(hwnd, &ps);
		return 0;
	}
	case WM_SETFONT:
	{
		mci->msg_font = (HFONT)wparam;
		if (LOWORD(lparam) == TRUE)
		{
			InvalidateRect(hwnd, NULL, TRUE);
		}
		return 0;
	}
	case MINERAL_CONTROL_MSG_SET_CELL:
	{
		mci->character = PACK_CELL_CHARACTER(wparam);
		mci->attrib = PACK_CELL_ATTRIBUTE(wparam);
		if (ATTRIBUTE_FOREGROUND(mci->attrib) == DNR_DIRT_INDEX || ATTRIBUTE_BACKGROUND(mci->attrib) == DNR_DIRT_INDEX)
		{
			mci->dirt_color = (COLORREF)lparam;
		}
		InvalidateRect(hwnd, NULL, TRUE);
		return 0;
	}
	case MINERAL_CONTROL_MSG_SET_INFO:
	{
		strncpy(mci->msg, (char*)wparam, sizeof mci->msg);
		InvalidateRect(hwnd, NULL, TRUE);
		return 0;
	}
	case WM_DESTROY:
		free((void*)GetWindowLongPtr(hwnd, GWLP_USERDATA));
		return 0;
	}
	return DefWindowProcW(hwnd, msg, wparam, lparam);
}

void mineral_control_initialize(void)
{
	WNDCLASSW class = { 0 };
	class.lpszClassName = MINERAL_CONTROL_CLASS_NAME;
	class.lpfnWndProc = mineral_control_window_proc;
	RUNTIME_ASSERT(RegisterClassW(&class));

	dnr_font = CreateFontA(-8, -8, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, OEM_CHARSET, OUT_DEVICE_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, DNR_FONT_A);
	RUNTIME_ASSERT(dnr_font);
}

void mineral_control_destroy(void)
{
	UnregisterClassW(MINERAL_CONTROL_CLASS_NAME, NULL);
	DeleteObject(dnr_font);
}

void mineral_control_set_info_f(HWND hwnd, const char* format, ...)
{
	char buf[2048];
	va_list args;

	va_start(args, format);
	vsprintf(buf, format, args);
	va_end(args);

	MINERAL_CONTROL_SET_INFO(hwnd, buf);
}