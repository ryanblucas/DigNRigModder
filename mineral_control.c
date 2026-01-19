/*
	mineral_control.c ~ RL
*/

#include "mineral_control.h"
#include "debug.h"
#include "screen.h"
#include "types.h"
#include <Windows.h>

struct mineral_control_internal
{
	char character;
	attribute_t attrib;
	COLORREF dirt_color;
	RECT rect;
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

		SelectObject(ps.hdc, dnr_font);
		SetTextColor(ps.hdc, mineral_control_get_color(mci, ATTRIBUTE_FOREGROUND(mci->attrib)));
		SetBkColor(ps.hdc, mineral_control_get_color(mci, ATTRIBUTE_BACKGROUND(mci->attrib)));
		TextOutA(ps.hdc, 16, 16, &mci->character, 1);

		EndPaint(hwnd, &ps);
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