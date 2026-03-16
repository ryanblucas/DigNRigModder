/*
	charmap_control.c ~ RL
	Creates a control for displaying a character map
*/

#include "charmap_control.h"
#include "../debug.h"
#include "../types.h"
#include <Windows.h>

#define CELL_MULTIPLIER 3
#define CELL_ACT_SIZE 8
#define CELL_ABS_SIZE (CELL_ACT_SIZE * CELL_MULTIPLIER)

struct charmap_control_window_data
{
	int selected_character;
};

static HFONT dnr_font;

static inline int charmap_control_calculate_abs_width(HWND hwnd)
{
	RECT target;
	GetClientRect(hwnd, &target);
	return ((target.right - target.left) / CELL_ABS_SIZE * CELL_ABS_SIZE);
}

static void charmap_control_recalculate_scroll(HWND hwnd)
{
	int height = charmap_control_calculate_abs_width(hwnd) / CELL_MULTIPLIER / CELL_ACT_SIZE;
	height = 0x100 / height;

	SCROLLINFO si = { .cbSize = sizeof si, .fMask = SIF_RANGE, .nMin = 0, .nMax = height - 1 };
	SetScrollInfo(hwnd, SB_VERT, &si, FALSE);
}

static LRESULT charmap_control_window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_CREATE:
	{
		struct charmap_control_window_data* ccwd = dig_malloc(sizeof * ccwd);
		ccwd->selected_character = -1;
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)ccwd);
		charmap_control_recalculate_scroll(hwnd);
		return 0;
	}
	case WM_DESTROY:
	{
		free((void*)GetWindowLongPtr(hwnd, GWLP_USERDATA));
		return 0;
	}
	case WM_LBUTTONDOWN:
	{
		int x = LOWORD(lparam), y = HIWORD(lparam);

		y += GetScrollPos(hwnd, SB_VERT) * CELL_ABS_SIZE;

		x /= CELL_ABS_SIZE;
		y /= CELL_ABS_SIZE;
		
		struct charmap_control_window_data* ccwd = (struct charmap_control_window_data*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		int width = charmap_control_calculate_abs_width(hwnd) / CELL_MULTIPLIER / CELL_ACT_SIZE;
		ccwd->selected_character = y * width + x;
		InvalidateRect(hwnd, NULL, TRUE);

		SendMessageW(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(hwnd), CHARMAP_CONTROL_CURRENT_SET), (LPARAM)hwnd);
		break;
	}
	case WM_VSCROLL:
	{
		SCROLLINFO si = { .cbSize = sizeof si };
		si.fMask = SIF_ALL;
		GetScrollInfo(hwnd, SB_VERT, &si);
		int y_pos = si.nPos;
		if (LOWORD(wparam))
		{
			si.nPos = si.nTrackPos;
		}
		si.fMask = SIF_POS;
		SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
		GetScrollInfo(hwnd, SB_VERT, &si);

		if (si.nPos != y_pos)
		{
			ScrollWindow(hwnd, 0, CELL_ACT_SIZE * (y_pos - si.nPos), NULL, NULL);
			InvalidateRect(hwnd, NULL, TRUE);
		}
		return 0;
	}
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		RECT target;
		GetClientRect(hwnd, &target);
		int rounded_width = charmap_control_calculate_abs_width(hwnd);
		int target_width = rounded_width / CELL_MULTIPLIER;

		HDC memory_dc = CreateCompatibleDC(ps.hdc);
		HBITMAP memory_bmp = CreateCompatibleBitmap(ps.hdc, target.right - target.left, target.bottom - target.top);

		SelectObject(memory_dc, memory_bmp);
		SelectObject(memory_dc, dnr_font);

		struct charmap_control_window_data* ccwd = (struct charmap_control_window_data*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		int x = 0, y = 0;
		for (int i = 0; i <= 0xFF; i++)
		{
			TextOutA(memory_dc, x, y, (char*)&i, 1);
			if (ccwd->selected_character == i)
			{
				BitBlt(memory_dc, x, y, CELL_ACT_SIZE, CELL_ACT_SIZE, NULL, 0, 0, DSTINVERT);
			}
			x += CELL_ACT_SIZE;
			if (x >= target_width)
			{
				x = 0;
				y += CELL_ACT_SIZE;
			}
		}

		int y_pos = GetScrollPos(hwnd, SB_VERT) * CELL_ACT_SIZE;
		StretchBlt(ps.hdc, 0, 0, rounded_width, y * CELL_MULTIPLIER + CELL_ABS_SIZE, memory_dc, 0, y_pos, target_width, y + CELL_ACT_SIZE, SRCCOPY);

		DeleteObject(memory_bmp);
		DeleteObject(memory_dc);

		EndPaint(hwnd, &ps);
		return 0;
	}
	case CHARMAP_CONTROL_MSG_SET_CURRENT:
	{
		struct charmap_control_window_data* ccwd = (struct charmap_control_window_data*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		ccwd->selected_character = (int)wparam;
		InvalidateRect(hwnd, NULL, TRUE);

		SendMessageW(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(hwnd), CHARMAP_CONTROL_CURRENT_SET), (LPARAM)hwnd);

		return 0;
	}
	case CHARMAP_CONTROL_MSG_GET_CURRENT:
	{
		struct charmap_control_window_data* ccwd = (struct charmap_control_window_data*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		return (LRESULT)ccwd->selected_character;
	}
	}
	return DefWindowProcW(hwnd, msg, wparam, lparam);
}

void charmap_control_initialize(void)
{
	WNDCLASSW class = { 0 };
	class.lpszClassName = CHARMAP_CONTROL_CLASS_NAME;
	class.lpfnWndProc = charmap_control_window_proc;
	RUNTIME_ASSERT(RegisterClassW(&class));

	dnr_font = CreateFontA(-8, -8, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, OEM_CHARSET, OUT_DEVICE_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, DNR_FONT_A);
	RUNTIME_ASSERT(dnr_font);
}

void charmap_control_destroy(void)
{
	UnregisterClassW(CHARMAP_CONTROL_CLASS_NAME, NULL);
	DeleteObject(dnr_font);
}