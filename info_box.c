/*
	info_box.c ~ RL
	Window that increases interactivity with the main display
*/

#include "info_box.h"
#include "debug.h"
#include "types.h"
#include <Windows.h>
#include <commctrl.h>

#define INFO_BOX_CLASS_NAME "dnr_mod_info"
#define INFO_BOX_WINDOW_STYLE (WS_OVERLAPPEDWINDOW)
#define INFO_BOX_WINDOW_STYLE_EX (WS_EX_OVERLAPPEDWINDOW)
#define INFO_BOX_START_CLIENT_WIDTH 300
#define INFO_BOX_START_CLIENT_HEIGHT 200

static HFONT font_caption;
static HFONT font_text;

static HWND window;
static HWND tab_control;

static HANDLE thread;
static DWORD thread_id;

static info_section_t* section_array;
static int section_count;

static LRESULT info_window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_CREATE:
	{
		tab_control = CreateWindowExA(0, WC_TABCONTROLA, NULL, WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS, 0, 0, INFO_BOX_START_CLIENT_WIDTH, INFO_BOX_START_CLIENT_HEIGHT, hwnd, NULL, NULL, NULL);
		RUNTIME_ASSERT(tab_control);

		for (int i = 0; i < section_count; i++)
		{
			TCITEMA tab = { .mask = TCIF_TEXT, .pszText = section_array[i].title };
			SendMessageA(tab_control, TCM_INSERTITEMA, i, (LPARAM)&tab);
		}

		SendMessageA(tab_control, WM_SETFONT, (WPARAM)font_caption, FALSE);
		return 0;
	}
	case WM_SIZE:
		SetWindowPos(tab_control, NULL, 0, 0, LOWORD(lparam), HIWORD(lparam), SWP_NOMOVE);
		return 0;
	case WM_CLOSE:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static void info_window_initialize(void)
{
	WNDCLASSA wc = { 0 };

	wc.lpfnWndProc = info_window_proc;
	wc.lpszClassName = INFO_BOX_CLASS_NAME;
	wc.hCursor = LoadCursorW(NULL, IDC_ARROW);

	RUNTIME_ASSERT(RegisterClassA(&wc));

	HWND console_window = GetConsoleWindow();
	RUNTIME_ASSERT(console_window);
	RECT console_window_bounds;
	RUNTIME_ASSERT(GetWindowRect(console_window, &console_window_bounds));
	/* This will probably go out of bounds if the console is on the right side of the screen */
	int x = console_window_bounds.right + 5;
	int y = console_window_bounds.top;

	RECT info_window_bounds = { .right = INFO_BOX_START_CLIENT_WIDTH, .bottom = INFO_BOX_START_CLIENT_HEIGHT };
	RUNTIME_ASSERT(AdjustWindowRectEx(&info_window_bounds, INFO_BOX_WINDOW_STYLE, FALSE, INFO_BOX_WINDOW_STYLE_EX));
	int wx = info_window_bounds.right - info_window_bounds.left;
	int wy = info_window_bounds.bottom - info_window_bounds.top;

	window = CreateWindowExA(INFO_BOX_WINDOW_STYLE_EX, INFO_BOX_CLASS_NAME, "Dig-N-Rig Modder", INFO_BOX_WINDOW_STYLE, x, y, wx, wy, NULL, NULL, NULL, NULL);
	RUNTIME_ASSERT(window);
}

static DWORD info_thread_proc(LPVOID param)
{
	info_window_initialize();
	ShowWindow(window, SHOW_OPENWINDOW);
	
	DWORD result = 0;
	MSG msg = { 0 };
	while (GetMessageA(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
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

void info_initialize(const info_section_t* _section_array, int _section_count)
{
	RUNTIME_ASSERT(_section_array && _section_count > 0);
	section_count = _section_count;
	section_array = dig_malloc(section_count * sizeof * section_array);
	memcpy(section_array, _section_array, section_count * sizeof * section_array);

	INITCOMMONCONTROLSEX icc = { .dwSize = sizeof icc, .dwICC = ICC_TAB_CLASSES };
	RUNTIME_ASSERT(InitCommonControlsEx(&icc));

	font_caption = CreateFontA(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_DONTCARE, "Arial");
	font_text = CreateFontA(11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_DONTCARE, "Arial");
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
	free(section_array);
}