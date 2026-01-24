/*
	info_box.c ~ RL
	Window that increases interactivity with the main display
*/

#include "info_box.h"
#include "debug.h"
#include "mineral_control.h"
#include "serialize.h"
#include "types.h"

#include <Windows.h>
#include <commctrl.h>
#include <strsafe.h>

#define INFO_BOX_CLASS_NAME L"dnr_mod_info"
#define INFO_BOX_WINDOW_STYLE (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX)
#define INFO_BOX_WINDOW_STYLE_EX (WS_EX_OVERLAPPEDWINDOW)
#define INFO_BOX_CLIENT_WIDTH 300
#define INFO_BOX_CLIENT_HEIGHT 400
#define INFO_BOX_CELL_SIZE 72

#define INFO_BOX_MSG_STATE_READY (WM_USER + 0x10)

static void info_state_set_tree_view(dnr_state_t* user_state);

enum child_window_index
{
	CWI_SAVE_TREEVIEW,
	CWI_SAVE_CURRENT_CELL,
	CWI_SAVE_PAINTER_CELL,

	CWI_COUNT
};

static HFONT font_caption;
static HFONT font_text;

static HWND window;
static HWND tab_control;

static HANDLE thread;
static DWORD thread_id;

static info_handle_change_mode change_mode_handler;
static info_mode_t current_mode;
static HWND child_windows[CWI_COUNT];

static dnr_state_t* state;

static inline void info_tab_create(const LPWSTR name, info_mode_t index)
{
	TCITEMW tab = { .mask = TCIF_TEXT, .pszText = name };
	TabCtrl_InsertItem(tab_control, index, &tab);
}

static void info_tab_save(void)
{
	if (!child_windows[CWI_SAVE_TREEVIEW])
	{
		RECT rect = { 0, 0, INFO_BOX_CLIENT_WIDTH, INFO_BOX_CLIENT_HEIGHT };
		TabCtrl_AdjustRect(tab_control, FALSE, &rect);

		int padding = ((198 - rect.top) - INFO_BOX_CELL_SIZE * 2) / 3;

		child_windows[CWI_SAVE_TREEVIEW] = CreateWindowExW(0, WC_TREEVIEWW, NULL, WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS, 2, 198, INFO_BOX_CLIENT_WIDTH - 4, 200, tab_control, NULL, NULL, NULL);
		RUNTIME_ASSERT(child_windows[CWI_SAVE_TREEVIEW]);
		SendMessageW(child_windows[CWI_SAVE_TREEVIEW], WM_SETFONT, (WPARAM)font_text, (LPARAM)FALSE);

		child_windows[CWI_SAVE_CURRENT_CELL] = CreateWindowExW(0, MINERAL_CONTROL_CLASS_NAME, NULL, WS_VISIBLE | WS_CHILD, padding, rect.top + padding, INFO_BOX_CLIENT_WIDTH / 3 * 2, 72, tab_control, NULL, NULL, NULL);
		RUNTIME_ASSERT(child_windows[CWI_SAVE_CURRENT_CELL]);
		SendMessageW(child_windows[CWI_SAVE_CURRENT_CELL], WM_SETFONT, (WPARAM)font_text, (LPARAM)FALSE);

		child_windows[CWI_SAVE_PAINTER_CELL] = CreateWindowExW(0, MINERAL_CONTROL_CLASS_NAME, NULL, WS_VISIBLE | WS_CHILD, padding, rect.top + padding * 2 + INFO_BOX_CELL_SIZE, INFO_BOX_CLIENT_WIDTH / 3 * 2, 72, tab_control, NULL, NULL, NULL);
		RUNTIME_ASSERT(child_windows[CWI_SAVE_PAINTER_CELL]);
	}

	ShowWindow(child_windows[CWI_SAVE_TREEVIEW], SW_SHOW);
	ShowWindow(child_windows[CWI_SAVE_CURRENT_CELL], SW_SHOW);
	ShowWindow(child_windows[CWI_SAVE_PAINTER_CELL], SW_SHOW);

	if (GetWindowLongPtr(window, GWLP_USERDATA) == 1)
	{
		SendMessageW(window, INFO_BOX_MSG_STATE_READY, (WPARAM)state, 0);
	}
}

static void info_tab_sprite(void)
{

}

static void info_tab_layer(void)
{

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

		info_tab_save();
		current_mode = MODE_SAVE;

		SendMessageW(tab_control, WM_SETFONT, (WPARAM)font_caption, FALSE);
		return 0;
	}
	case WM_NOTIFY:
	{
		NMHDR* nmhdr = (NMHDR*)lparam;
		if (nmhdr->hwndFrom != tab_control || nmhdr->code != TCN_SELCHANGE)
		{
			return 0;
		}
		current_mode = TabCtrl_GetCurSel(tab_control);
		for (int i = 0; i < CWI_COUNT; i++)
		{
			ShowWindow(child_windows[i], SW_HIDE);
		}
		switch (current_mode)
		{
		case MODE_SAVE:
			info_tab_save();
			break;
		case MODE_SPRITE:
			info_tab_sprite();
			break;
		case MODE_LAYER:
			info_tab_layer();
			break;
		default:
			RUNTIME_ASSERT(false);
			break;
		}
		if (change_mode_handler)
		{
			change_mode_handler(current_mode);
		}
		return 0;
	}
	case INFO_BOX_MSG_STATE_READY:
		if (!child_windows[CWI_SAVE_TREEVIEW])
		{
			SetWindowLongPtr(hwnd, GWLP_USERDATA, 1);
			return 0;
		}
		info_state_set_tree_view((dnr_state_t*)wparam);
		return 0;
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
	WNDCLASSW wc = { 0 };

	wc.lpfnWndProc = info_window_proc;
	wc.lpszClassName = INFO_BOX_CLASS_NAME;
	wc.hCursor = LoadCursorW(NULL, IDC_ARROW);

	RUNTIME_ASSERT(RegisterClassW(&wc));

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

	/* why is passing a wide string for title wrong here? */
	window = CreateWindowExW(INFO_BOX_WINDOW_STYLE_EX, INFO_BOX_CLASS_NAME, "Dig-N-Rig Modder", INFO_BOX_WINDOW_STYLE, x, y, wx, wy, NULL, NULL, NULL, NULL);
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

void info_initialize(info_handle_change_mode handler)
{
	change_mode_handler = handler;

	INITCOMMONCONTROLSEX icc = { .dwSize = sizeof icc, .dwICC = ICC_TREEVIEW_CLASSES | ICC_TAB_CLASSES };
	RUNTIME_ASSERT(InitCommonControlsEx(&icc));
	mineral_control_initialize();

	font_caption = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_DONTCARE, L"Arial");
	font_text = CreateFontW(-10, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_DONTCARE, L"Arial");
	RUNTIME_ASSERT(font_caption && font_text);

	thread = CreateThread(NULL, 0, info_thread_proc, NULL, 0, &thread_id);
	RUNTIME_ASSERT(thread);
	debug_format("Created thread %i for info box\n", thread_id);
}

void info_destroy(void)
{
	mineral_control_destroy();

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

dnr_state_t* info_state_get(void)
{
	return state;
}

static void info_state_set_tree_view(dnr_state_t* user_state)
{
	debug_profiler_push();
	dnr_state_t* item = user_state;

	/* array is set to mininum of 1 because you are essentially creating a preview of the array. 
	   If everything is initialized from the beginning, it will take like 10 minutes and 4Gb. */

#define ADD_SERIALIZABLE(type, name) serialize_single(#type, &item->name, #name, child_windows[CWI_SAVE_TREEVIEW], root);
#define ADD_SERIALIZABLE_ARRAY(type, name, count) serialize_array(#type, &item->name, 1, #name, child_windows[CWI_SAVE_TREEVIEW], root);

	HTREEITEM root = NULL;

	SERIALIZABLE_DNR_STATE_0

	TVINSERTSTRUCTW tvins;

	tvins.hParent = TVI_ROOT;
	tvins.itemex.pszText = L"stalactite_array";
	tvins.itemex.mask = TVIF_TEXT;
	tvins.hInsertAfter = TVI_LAST;
	root = TreeView_InsertItem(child_windows[CWI_SAVE_TREEVIEW], &tvins);
	RUNTIME_ASSERT(root);

	tvins.hParent = root;

	for (int i = 0; i < user_state->stalactite_count; i++)
	{
		stalactite_t* item = &user_state->stalactite_array[i];

		WCHAR buf[8];
		tvins.itemex.pszText = buf;
		StringCchPrintfW(tvins.itemex.pszText, sizeof buf / sizeof * buf, L"%i\n", i);
		root = TreeView_InsertItem(child_windows[CWI_SAVE_TREEVIEW], &tvins);
		RUNTIME_ASSERT(root);

		SERIALIZABLE_STALACTITE
	}

	root = NULL;

	SERIALIZABLE_DNR_STATE_1

#undef ADD_SERIALIZABLE
#undef ADD_SERIALIZABLE_ARRAY

	debug_profiler_pop("Serializing");
}

void info_state_set(dnr_state_t* _state)
{
	state = _state;
		
	if (current_mode == MODE_SAVE)
	{
		/* i do not like this */
		for (int i = 0; i < 100 && !window; i++)
		{
			Sleep(10);
		}
		RUNTIME_ASSERT(window);
		SendMessageW(window, INFO_BOX_MSG_STATE_READY, (WPARAM)state, 0);
	}
}

void info_cell_set_current(int x, int y)
{
	if (!state)
	{
		debug_format("Tried to set current cell with no state\n");
		return;
	}

	if (x == -1 && y == -1)
	{
		MINERAL_CONTROL_SET_CELL(child_windows[CWI_SAVE_CURRENT_CELL], 0, 0, 0);
		return;
	}

	RUNTIME_ASSERT(x >= 0 && y >= 0 && x < TARGET_WIDTH && y < TARGET_HEIGHT * LAYER_COUNT);

	CHAR_INFO cell = file_state_spritify_cell(state, x, y);
	int layer_index = y / TARGET_HEIGHT;
	MINERAL_CONTROL_SET_CELL(child_windows[CWI_SAVE_CURRENT_CELL], cell.Char.AsciiChar, cell.Attributes, state->layer_headers[layer_index].dirt_color);
}