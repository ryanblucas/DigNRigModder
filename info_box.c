/*
	info_box.c ~ RL
	Window that increases interactivity with the main display
*/

#include "info_box.h"
#include "debug.h"
#include "mineral_control.h"
#include "types.h"
#include <Windows.h>
#include <commctrl.h>

#define INFO_BOX_CLASS_NAME L"dnr_mod_info"
#define INFO_BOX_WINDOW_STYLE (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX)
#define INFO_BOX_WINDOW_STYLE_EX (WS_EX_OVERLAPPEDWINDOW)
#define INFO_BOX_CLIENT_WIDTH 300
#define INFO_BOX_CLIENT_HEIGHT 400
#define INFO_BOX_CELL_SIZE 72

enum child_window_index
{
	CWI_SAVE_TREEVIEW,
	CWI_SAVE_CURRENT_CELL,
	CWI_SAVE_PAINTER_CELL
};

static HFONT font_caption;
static HFONT font_text;

static HWND window;
static HWND tab_control;

static HANDLE thread;
static DWORD thread_id;

static info_handle_change_mode change_mode_handler;
static info_mode_t current_mode;
static HWND child_windows[MODE_COUNT][4];

static dnr_state_t* state;

static inline void info_tab_create(const LPWSTR name, info_mode_t index)
{
	TCITEMW tab = { .mask = TCIF_TEXT, .pszText = name };
	TabCtrl_InsertItem(tab_control, index, &tab);
}

static void info_tab_save(void)
{
	if (!child_windows[MODE_SAVE][0])
	{
		RECT rect = { 0, 0, INFO_BOX_CLIENT_WIDTH, INFO_BOX_CLIENT_HEIGHT };
		TabCtrl_AdjustRect(tab_control, FALSE, &rect);

		int padding = ((198 - rect.top) - INFO_BOX_CELL_SIZE * 2) / 3;

		child_windows[MODE_SAVE][0] = CreateWindowExW(0, WC_TREEVIEWW, NULL, WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | TVS_HASLINES, 2, 198, INFO_BOX_CLIENT_WIDTH - 4, 200, tab_control, NULL, NULL, NULL);
		RUNTIME_ASSERT(child_windows[MODE_SAVE][CWI_SAVE_TREEVIEW]);

		child_windows[MODE_SAVE][1] = CreateWindowExW(0, MINERAL_CONTROL_CLASS_NAME, NULL, WS_VISIBLE | WS_CHILD, padding, rect.top + padding, INFO_BOX_CLIENT_WIDTH / 3 * 2, 72, tab_control, NULL, NULL, NULL);
		RUNTIME_ASSERT(child_windows[MODE_SAVE][CWI_SAVE_CURRENT_CELL]);

		child_windows[MODE_SAVE][2] = CreateWindowExW(0, MINERAL_CONTROL_CLASS_NAME, NULL, WS_VISIBLE | WS_CHILD, padding, rect.top + padding * 2 + INFO_BOX_CELL_SIZE, INFO_BOX_CLIENT_WIDTH / 3 * 2, 72, tab_control, NULL, NULL, NULL);
		RUNTIME_ASSERT(child_windows[MODE_SAVE][CWI_SAVE_PAINTER_CELL]);

		SendMessageW(child_windows[MODE_SAVE][1], WM_SETFONT, (WPARAM)font_text, (LPARAM)FALSE);
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

void info_state_set(dnr_state_t* _state)
{
	state = _state;
	if (current_mode == MODE_SAVE)
	{
		UpdateWindow(window);
	}
}

/* by running serialize_hash on each of the types as a string, you get this result. */

#define TYPE_FLOAT 210624726069ULL
#define TYPE_BOOLEAN32_T 13766221191973021547ULL
#define TYPE_CHAR_INFO 249834764690065676ULL
#define TYPE_INT32_T 229378475688636ULL
#define TYPE_UINT32_T 7569686425136137ULL
#define TYPE_DNR_POINTER_T 15207900801384124882ULL
#define TYPE_UINT8_T 229384437135984ULL
#define TYPE_DNR_SPRITE_T 11081697800589051136ULL
#define TYPE_DNR_RIG_TYPE_T 3798355938377845426ULL
#define TYPE_DNR_SAVE_HEADER_T 12164599792533459048ULL
#define TYPE_DNR_LAYER_HEADER_T 663611519707857130ULL
#define TYPE_DNR_PLAYER_T 11081698126627463866ULL
#define TYPE_DNR_BLOCK_T 13751622877662047520ULL
#define TYPE_DNR_MINERAL_T 15207893016492402041ULL

extern inline uint64_t serialize_hash(const char* str)
{
	uint64_t hash = 5381;
	do
	{
		hash = ((hash << 5) + hash) ^ *str;
	} while (*++str);
	return hash;
}

static void serialize_array(const char* type, const char* name, void* value, int count)
{
	uint64_t type_hash = serialize_hash(type);
	while (count > 0)
	{
		switch (type_hash)
		{
		case TYPE_FLOAT:
			debug_format("%s - %f\n", name, *(float*)value);
			value = (float*)value + 1;
			count--;
			break;
		case TYPE_BOOLEAN32_T:
		case TYPE_INT32_T:
			debug_format("%s - %i\n", name, *(int32_t*)value);
			value = (int32_t*)value + 1;
			count--;
			break;
		case TYPE_CHAR_INFO:
		{
			CHAR_INFO ci_value = *(CHAR_INFO*)value;
			debug_format("%s - {char: %#04X, attributes: %#06X}\n", name, ci_value.Char.AsciiChar, ci_value.Attributes);
			value = (CHAR_INFO*)value + 1;
			count--;
			break;
		}
		case TYPE_UINT32_T:
		case TYPE_DNR_POINTER_T:
			debug_format("%s - %ui\n", name, *(uint32_t*)value);
			value = (uint32_t*)value + 1;
			count--;
			break;
		case TYPE_UINT8_T:
			debug_format("%s - %#02X\n", name, *(uint8_t*)value);
			value = (uint8_t*)value + 1;
			count--;
			break;
		case TYPE_DNR_RIG_TYPE_T:
#define ADD_SERIALIZABLE_ENUM(enum_name, enum_value) case enum_value: debug_format("%s - " #enum_name "\n", name); break;
			switch (*(dnr_rig_type_t*)value)
			{
				SERIALIZABLE_DNR_RIG_TYPE
			}
#undef ADD_SERIALIZABLE_ENUM
			value = (dnr_rig_type_t*)value + 1;
			count--;
			break;
		default:
			RUNTIME_ASSERT(false);
		}
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
		MINERAL_CONTROL_SET_CELL(child_windows[MODE_SAVE][CWI_SAVE_CURRENT_CELL], 0, 0, 0);
		return;
	}

	RUNTIME_ASSERT(x >= 0 && y >= 0 && x < TARGET_WIDTH && y < TARGET_HEIGHT * LAYER_COUNT);

	CHAR_INFO cell = file_state_spritify_cell(state, x, y);
	int layer_index = y / TARGET_HEIGHT;
	MINERAL_CONTROL_SET_CELL(child_windows[MODE_SAVE][CWI_SAVE_CURRENT_CELL], cell.Char.AsciiChar, cell.Attributes, state->layer_headers[layer_index].dirt_color);

	dnr_block_t* block = &state->blocks[x * (TARGET_HEIGHT * LAYER_COUNT) + y];

#define ADD_SERIALIZABLE(type, name) serialize_array(#type, #name, &block->name, 1);
#define ADD_SERIALIZABLE_ARRAY(type, name, count) serialize_array(#type, #name, &block->name, count);

	SERIALIZABLE_DNR_BLOCK

#undef ADD_SERIALIZABLE
#undef ADD_SERIALIZABLE_ARRAY
}