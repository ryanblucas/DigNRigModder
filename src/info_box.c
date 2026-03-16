/*
	info_box.c ~ RL
	Window that increases interactivity with the main display
*/

#include "info_box.h"
#include "debug.h"
#include "types.h"

#include "save_viewer/save_info.h"

#include <Windows.h>
#include <commctrl.h>
#include <strsafe.h>

#define INFO_BOX_CLASS_NAME L"dnr_mod_info"
#define INFO_BOX_WINDOW_STYLE (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX)
#define INFO_BOX_WINDOW_STYLE_EX (WS_EX_OVERLAPPEDWINDOW)

static HFONT font_caption; 
static HFONT font_text;

static HWND window;
static HWND tab_control;

static HANDLE thread;
static DWORD thread_id;

static info_mode_t current_mode;

static info_events_t events;

static inline void info_tab_create(const LPWSTR name, info_mode_t index)
{
	TCITEMW tab = { .mask = TCIF_TEXT, .pszText = name };
	TabCtrl_InsertItem(tab_control, index, &tab);
}

static LRESULT info_window_tab_control_proc(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
	NMHDR* nmhdr = (NMHDR*)lparam;
	if (nmhdr->code != TCN_SELCHANGE)
	{
		return 0;
	}
	if (current_mode == MODE_SAVE)
	{
		save_info_show(false);
	}
	current_mode = TabCtrl_GetCurSel(tab_control);
	switch (current_mode)
	{
	case MODE_SAVE:
		save_info_show(true);
		break;
	default:
		RUNTIME_ASSERT(false);
		break;
	}
	RAISE_EVENT(events.mode_handler, current_mode);
	return 0;
}

static LRESULT info_window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_CREATE:
	{
		tab_control = CreateWindowExW(0, WC_TABCONTROLW, NULL, WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS, 0, 0, INFO_BOX_CLIENT_WIDTH, INFO_BOX_CLIENT_HEIGHT, hwnd, NULL, NULL, NULL);
		RUNTIME_ASSERT(tab_control);

		info_tab_create(L"Save", MODE_SAVE);
		info_tab_create(L"Sprite", MODE_SPRITE);
		info_tab_create(L"Layer", MODE_LAYER);

		info_internal_t internal = { .window = hwnd, .events = &events, .font_caption = font_caption, .font_text = font_text };
		save_info_initialize(&internal);
		save_info_show(true);
		current_mode = MODE_SAVE;

		SendMessageW(tab_control, WM_SETFONT, (WPARAM)font_caption, FALSE);
		return 0;
	}
	case WM_NOTIFY:
	{
		NMHDR* nmhdr = (NMHDR*)lparam;
		if (nmhdr->hwndFrom == tab_control)
		{
			return info_window_tab_control_proc(hwnd, wparam, lparam);
		}
		break;
	}
	case WM_SIZE:
		SetWindowPos(tab_control, NULL, 0, 0, LOWORD(lparam), HIWORD(lparam), SWP_NOMOVE);
		return 0;
	case WM_CLOSE:
		PostQuitMessage(0);
		return 0;
	}
	bool use_result = false;
	LRESULT result;
	if (current_mode == MODE_SAVE)
	{
		use_result = save_info_proc(hwnd, msg, wparam, lparam, &result);
	}
	return use_result ? result : DefWindowProcW(hwnd, msg, wparam, lparam);
}

static void info_window_initialize(void)
{
	WNDCLASSEXW wc = { 0 };

	wc.cbSize = sizeof wc;
	wc.lpfnWndProc = info_window_proc;
	wc.lpszClassName = INFO_BOX_CLASS_NAME;
	wc.hCursor = LoadCursorW(NULL, IDC_ARROW);

	RUNTIME_ASSERT(RegisterClassExW(&wc));

	HWND console_window = GetConsoleWindow();
	RUNTIME_ASSERT(console_window);
	RECT console_window_bounds;
	RUNTIME_ASSERT(GetWindowRect(console_window, &console_window_bounds));
	/* This will probably go out of bounds if the console is on the right side of the screen */
	int x = console_window_bounds.right + 5;
	int y = console_window_bounds.top;

	RECT info_window_bounds = { .right = INFO_BOX_CLIENT_WIDTH, .bottom = INFO_BOX_CLIENT_HEIGHT };
	RUNTIME_ASSERT(AdjustWindowRectEx(&info_window_bounds, INFO_BOX_WINDOW_STYLE, FALSE, INFO_BOX_WINDOW_STYLE_EX));
	int wx = info_window_bounds.right - info_window_bounds.left;
	int wy = info_window_bounds.bottom - info_window_bounds.top;

	window = CreateWindowExW(INFO_BOX_WINDOW_STYLE_EX, INFO_BOX_CLASS_NAME, L"Dig-N-Rig Modder", INFO_BOX_WINDOW_STYLE, x, y, wx, wy, NULL, NULL, NULL, NULL);
	RUNTIME_ASSERT(window);
}

static DWORD info_thread_proc(LPVOID param)
{
	info_window_initialize();
	ShowWindow(window, SHOW_OPENWINDOW);
	SetForegroundWindow(GetConsoleWindow());
	
	DWORD result = 0;
	MSG msg = { 0 };
	while (GetMessageW(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
		if (msg.message == WM_QUIT)
		{
			result = (DWORD)msg.wParam;
		}
	}
	window = NULL;
	FreeConsole();
	/* with FreeConsole() being called here, it's not great programming, but the main method will 
		call info_destroy and clean up resources used here instead of doing it right here.
		Otherwise, I'd have to find some way to free some memory allocated in another thread */
	return result;
}

void info_initialize(info_events_t _events)
{
	events = _events;

	INITCOMMONCONTROLSEX icc = { .dwSize = sizeof icc, .dwICC = ICC_TREEVIEW_CLASSES | ICC_TAB_CLASSES };
	RUNTIME_ASSERT(InitCommonControlsEx(&icc));

	font_caption = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_DONTCARE, L"Arial");
	font_text = CreateFontW(-10, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_DONTCARE, L"Arial");
	RUNTIME_ASSERT(font_caption && font_text);

	thread = CreateThread(NULL, 0, info_thread_proc, NULL, 0, &thread_id);
	RUNTIME_ASSERT(thread);
	debug_format("Created thread %i for info box\n", thread_id);
}

void info_destroy(void)
{
	if (window)
	{
		PostQuitMessage(0);
	}
	if (GetConsoleWindow())
	{
		FreeConsole();
	}
	CloseHandle(thread);
	DeleteObject(font_caption);
	DeleteObject(font_text);
	UnregisterClassW(INFO_BOX_CLASS_NAME, NULL);
}

info_mode_t info_get_current_mode(void)
{
	return current_mode;
}