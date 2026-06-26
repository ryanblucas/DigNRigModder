/*
	info_box.c ~ RL
	Window that increases interactivity with the main display
*/

#include "info_box.h"
#include "debug.h"
#include "interface/resource.h"
#include "types.h"
#include "path.h"
#include <Windows.h>
#include <commctrl.h>
#include <strsafe.h>

#define INFO_BOX_CLASS_NAME L"dnr_mod_info"
#define INFO_BOX_WINDOW_STYLE (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX)
#define INFO_BOX_WINDOW_STYLE_EX (WS_EX_OVERLAPPEDWINDOW)

#define INFO_BOX_CAPTION L"Dig-N-Rig Modder"
#define INFO_BOX_CAPTION_MAX_SIZE 128

static HFONT font_caption; 
static HFONT font_text;

static HWND window;
static HWND tab_control;

static HANDLE thread;
static DWORD thread_id;

static info_mode_t current_mode;
static info_mode_class_t classes[MODE_COUNT];
static HWND global_treeviews[MODE_COUNT];
static HWND current_treeviews[MODE_COUNT];

static WCHAR captions[INFO_BOX_CAPTION_MAX_SIZE * MODE_COUNT];

static info_events_t events;
static info_handle_change_mode_t change_mode;

static inline void info_tab_create(const char* name, info_mode_t index)
{
	WCHAR wname[64];
	RUNTIME_ASSERT(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, name, -1, wname, sizeof wname / sizeof * wname));
	TCITEMW tab = { .mask = TCIF_TEXT, .pszText = wname };
	TabCtrl_InsertItem(tab_control, index, &tab);
}

static void info_window_tree_control_proc(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
	NMTREEVIEWW* nmtv = (NMTREEVIEWW*)lparam;
	if (nmtv->hdr.code == TVN_ITEMEXPANDING && nmtv->action == TVE_EXPAND && nmtv->itemNew.mask & TVIF_PARAM)
	{
		serialize_on_expand((element_t)nmtv->itemNew.lParam);
	}
	else if (nmtv->hdr.code == NM_DBLCLK)
	{
		/* the LPARAM here is NOT a NMTREEVIEWW */
		TVHITTESTINFO info;
		GetCursorPos(&info.pt);
		ScreenToClient(nmtv->hdr.hwndFrom, &info.pt);

		HTREEITEM res = TreeView_HitTest(nmtv->hdr.hwndFrom, &info);
		if (res && info.flags & TVHT_ONITEM)
		{
			TVITEMEXW tvi;
			tvi.hItem = res;
			tvi.mask = TVIF_PARAM;
			TreeView_GetItem(nmtv->hdr.hwndFrom, &tvi);
			element_t element = (element_t)tvi.lParam;

			if (!element)
			{
				return;
			}

			if (classes[current_mode].interact_tree_item 
				&& !classes[current_mode].interact_tree_item(nmtv->hdr.hwndFrom == global_treeviews[current_mode], element))
			{
				MessageBeep(MB_ICONERROR);
			}
		}
	}
}

static LRESULT info_window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case QUEUE_MSG_MAIN_TO_WINDOW:
	{
		handle_generic_t handler = (handle_generic_t)wparam;
		const void* data = (const void*)lparam;
		handler(data);
		return 0;
	}
	case WM_CREATE:
	{
		tab_control = CreateWindowExW(0, WC_TABCONTROLW, NULL, WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS, 0, 0, INFO_BOX_CLIENT_WIDTH, INFO_BOX_CLIENT_HEIGHT, hwnd, NULL, NULL, NULL);
		RUNTIME_ASSERT(tab_control);

		for (int i = 0; i < MODE_COUNT; i++)
		{
			/* just a check to see if the class exists */
			RUNTIME_ASSERT(classes[i].initialize);

			info_tab_create(classes[i].caption, i);
			info_internal_t internal = { .window = hwnd, .events = &events, .font_caption = font_caption, .font_text = font_text };
			classes[i].initialize(&internal);
			global_treeviews[i] = internal.global_treeview;
			current_treeviews[i] = internal.current_treeview;
		}

		classes[current_mode].show(true);

		SendMessageW(tab_control, WM_SETFONT, (WPARAM)font_caption, FALSE);
		return 0;
	}
	case WM_NOTIFY:
	{
		NMHDR* nmhdr = (NMHDR*)lparam;
		if (nmhdr->hwndFrom == tab_control && nmhdr->code == TCN_SELCHANGE)
		{
			info_set_current_mode(TabCtrl_GetCurSel(tab_control));
		}
		else if (nmhdr->hwndFrom == global_treeviews[current_mode] || nmhdr->hwndFrom == current_treeviews[current_mode])
		{
			info_window_tree_control_proc(hwnd, wparam, lparam);
		}
		break;
	}
	case WM_KEYUP:
	case WM_KEYDOWN:
	case WM_SYSKEYUP:
	case WM_SYSKEYDOWN:
	{
		INPUT_RECORD ir = { .EventType = KEY_EVENT };
		ir.Event.KeyEvent = (KEY_EVENT_RECORD)
		{
			.bKeyDown = msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN,
			.wVirtualKeyCode = (WORD)wparam,
			.wRepeatCount = (WORD)(lparam & 0xFFFF),
			.wVirtualScanCode = (WORD)((lparam >> 16) & 0xFF)
		};

		BYTE keyboard_state[256];
		RUNTIME_ASSERT(GetKeyboardState(keyboard_state));
		wchar_t ch[2] = { 0 };

		ToUnicode((UINT)wparam, ir.Event.KeyEvent.wVirtualScanCode,
			keyboard_state, ch, 2, 0);
		ir.Event.KeyEvent.uChar.UnicodeChar = ch[0];

		ir.Event.KeyEvent.dwControlKeyState |= (GetKeyState(VK_SHIFT) & 0x8000) ? SHIFT_PRESSED : 0;
		ir.Event.KeyEvent.dwControlKeyState |= (GetKeyState(VK_CONTROL) & 0x8000) ? LEFT_CTRL_PRESSED : 0;
		ir.Event.KeyEvent.dwControlKeyState |= (GetKeyState(VK_MENU) & 0x8000) ? LEFT_ALT_PRESSED : 0;
		ir.Event.KeyEvent.dwControlKeyState |= (GetKeyState(VK_CAPITAL) & 0x0001) ? CAPSLOCK_ON : 0;
		ir.Event.KeyEvent.dwControlKeyState |= (GetKeyState(VK_NUMLOCK) & 0x0001) ? NUMLOCK_ON : 0;
		ir.Event.KeyEvent.dwControlKeyState |= (GetKeyState(VK_SCROLL) & 0x0001) ? SCROLLLOCK_ON : 0;

		DWORD written;
		RUNTIME_ASSERT(WriteConsoleInputW(GetStdHandle(STD_INPUT_HANDLE), &ir, 1, &written) && written == 1);
		return 0;
	}
	case WM_SIZE:
		SetWindowPos(tab_control, NULL, 0, 0, LOWORD(lparam), HIWORD(lparam), SWP_NOMOVE);
		return 0;
	case WM_CLOSE:
		PostQuitMessage(0);
		return 0;
	}
	LRESULT result;
	return classes[current_mode].proc(hwnd, msg, wparam, lparam, &result) ? result : DefWindowProcW(hwnd, msg, wparam, lparam);
}

static void info_window_initialize(void)
{
	WNDCLASSEXW wc = { 0 };

	wc.cbSize = sizeof wc;
	wc.lpfnWndProc = info_window_proc;
	wc.lpszClassName = INFO_BOX_CLASS_NAME;
	wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
	wc.hIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR | LR_SHARED);
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

	window = CreateWindowExW(INFO_BOX_WINDOW_STYLE_EX, INFO_BOX_CLASS_NAME, INFO_BOX_CAPTION, INFO_BOX_WINDOW_STYLE, x, y, wx, wy, NULL, NULL, NULL, NULL);
	RUNTIME_ASSERT(window);

	for (int i = 0; i < MODE_COUNT; i++)
	{
		wcsncpy(&captions[i * INFO_BOX_CAPTION_MAX_SIZE], INFO_BOX_CAPTION, INFO_BOX_CAPTION_MAX_SIZE);
	}
}

static DWORD info_thread_proc(LPVOID param)
{
	info_window_initialize();
	queue_set_window_handle(window);
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
	debug_format("Closed from window\n");
	return result;
}

void info_set_caption(const char* caption)
{
	if (!caption || !*caption)
	{
		wcsncpy(&captions[current_mode * INFO_BOX_CAPTION_MAX_SIZE], INFO_BOX_CAPTION, INFO_BOX_CAPTION_MAX_SIZE);
		SetWindowTextW(window, INFO_BOX_CAPTION);
		return;
	}
	WCHAR wname[min(64, INFO_BOX_CAPTION_MAX_SIZE)], buf[INFO_BOX_CAPTION_MAX_SIZE];
	RUNTIME_ASSERT(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, caption, -1, wname, sizeof wname / sizeof * wname));
	StringCbPrintfW(buf, sizeof buf, INFO_BOX_CAPTION " - %s", wname);
	SetWindowTextW(window, buf);
	wcsncpy(&captions[current_mode * INFO_BOX_CAPTION_MAX_SIZE], buf, INFO_BOX_CAPTION_MAX_SIZE);
}

void info_add_class(const info_mode_class_t* class)
{
	RUNTIME_ASSERT(class->mode >= 0 && class->mode < MODE_COUNT);
	classes[class->mode] = *class;
}

void info_set_event_handlers(const info_events_t* _events)
{
	events = *_events;
}

void info_initialize(info_handle_change_mode_t _change_mode)
{
	change_mode = _change_mode;
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
	CloseHandle(thread);
	thread = NULL;
	if (window)
	{
		PostQuitMessage(0);
		window = NULL;
	}
	if (GetConsoleWindow())
	{
		FreeConsole();
	}
	DeleteObject(font_caption);
	DeleteObject(font_text);
	font_caption = NULL;
	font_text = NULL;
	UnregisterClassW(INFO_BOX_CLASS_NAME, NULL);
}

info_mode_t info_get_current_mode(void)
{
	return current_mode;
}

void info_set_current_mode(info_mode_t mode)
{
	for (int i = 0; i < 100 && TabCtrl_GetItemCount(tab_control) != MODE_COUNT; i++)
	{
		Sleep(10);
	}
	if (mode == current_mode)
	{
		return;
	}
	RUNTIME_ASSERT((int)mode >= 0 && (int)mode < MODE_COUNT);
	TabCtrl_SetCurSel(tab_control, mode);
	classes[current_mode].show(false);
	info_mode_t prev = current_mode;
	current_mode = mode;
	RAISE_EVENT(change_mode, prev);
	classes[mode].show(true);

	SetWindowTextW(window, &captions[mode * INFO_BOX_CAPTION_MAX_SIZE]);
}