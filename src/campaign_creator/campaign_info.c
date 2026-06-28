/*
	campaign_info.c ~ RL
*/

#include "campaign_info.h"
#include "../path.h"

enum child_window_index
{
	CWI_LISTBOX_LAYER_FILES,
	CWI_LISTBOX_LAYER_NAMES,

	CWI_TEXTBOX_TITLE,
	CWI_TEXT_TITLE,

	CWI_BUTTON_SHOW_ENDBOX_TOGGLE,
	CWI_BUTTON_SHOW_BINARY_TOGGLE,
	CWI_BUTTON_WORK_BINARY_TOGGLE,

	CWI_BUTTON_ERASER,
	CWI_BUTTON_SELECT,
	CWI_BUTTON_BRUSH,
	CWI_THUMB_BRUSH_SIZE,
	CWI_BUTTON_RECTANGLE,

	CWI_BUTTON_FILE_SELECTER,
	CWI_TREEVIEW,
	CWI_CURRENT_TREEVIEW,

	CWI_COUNT
};

static campaign_t* campaign;
static HWND child_windows[CWI_COUNT];
static info_internal_t internal;

void campaign_info_initialize(info_internal_t* _internal)
{
	internal = *_internal;

	child_windows[CWI_TEXT_TITLE] = CreateWindowExW(0, L"STATIC", L"Title: ", WS_CHILD | WS_OVERLAPPED | WS_VISIBLE, 82, 33, 30, 14, internal.window, NULL, NULL, NULL);
	child_windows[CWI_TEXTBOX_TITLE] = CreateWindowExW(0, L"EDIT", NULL, WS_CHILD | WS_OVERLAPPED | WS_VISIBLE | WS_BORDER, 112, 29, 102, 22, internal.window, NULL, NULL, NULL);
	SendMessageW(child_windows[CWI_TEXT_TITLE], WM_SETFONT, (WPARAM)internal.font_caption, 0);
	SendMessageW(child_windows[CWI_TEXTBOX_TITLE], WM_SETFONT, (WPARAM)internal.font_caption, 0);

	child_windows[CWI_BUTTON_FILE_SELECTER] = CreateWindowExW(0, L"BUTTON", L"Find file", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 8, 29, 62, 22, internal.window, NULL, NULL, NULL);
	SendMessageW(child_windows[CWI_BUTTON_FILE_SELECTER], WM_SETFONT, (WPARAM)internal.font_text, 0);

	child_windows[CWI_LISTBOX_LAYER_FILES] = CreateWindowExW(0, L"LISTBOX", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_NOTIFY | WS_VSCROLL, 225, 24, 111, 177, internal.window, NULL, NULL, NULL);
	child_windows[CWI_LISTBOX_LAYER_NAMES] = CreateWindowExW(0, L"LISTBOX", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_NOTIFY | WS_VSCROLL, 336, 24, 111, 177, internal.window, NULL, NULL, NULL);
	SendMessageW(child_windows[CWI_LISTBOX_LAYER_FILES], WM_SETFONT, (WPARAM)internal.font_text, (LPARAM)FALSE);
	SendMessageW(child_windows[CWI_LISTBOX_LAYER_NAMES], WM_SETFONT, (WPARAM)internal.font_text, (LPARAM)FALSE);

	child_windows[CWI_BUTTON_ERASER] = CreateWindowExW(0, L"BUTTON", L"Erase", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 3, 75, 72, 22, internal.window, NULL, NULL, NULL);
	child_windows[CWI_BUTTON_SELECT] = CreateWindowExW(0, L"BUTTON", L"Select", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 3, 98, 72, 22, internal.window, NULL, NULL, NULL);
	child_windows[CWI_BUTTON_BRUSH] = CreateWindowExW(0, L"BUTTON", L"Brush", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 3, 121, 72, 22, internal.window, NULL, NULL, NULL);
	child_windows[CWI_THUMB_BRUSH_SIZE] = CreateWindowExW(0, TRACKBAR_CLASSW, L"Brush size", WS_VISIBLE | WS_CHILD | TBS_AUTOTICKS | TBS_ENABLESELRANGE, 3, 144, 72, 22, internal.window, NULL, NULL, NULL);
	SendMessageW(child_windows[CWI_THUMB_BRUSH_SIZE], TBM_SETRANGE, TRUE, MAKELONG(INFO_BRUSH_MIN_SIZE, INFO_BRUSH_MAX_SIZE));
	child_windows[CWI_BUTTON_RECTANGLE] = CreateWindowExW(0, L"BUTTON", L"Set end box", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 3, 167, 72, 22, internal.window, NULL, NULL, NULL);

	child_windows[CWI_BUTTON_SHOW_ENDBOX_TOGGLE] = CreateWindowExW(0, L"BUTTON", L"Show endbox", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 82, 75, 132, 22, internal.window, NULL, NULL, NULL);
	child_windows[CWI_BUTTON_SHOW_BINARY_TOGGLE] = CreateWindowExW(0, L"BUTTON", L"Show binary", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 82, 98, 132, 22, internal.window, NULL, NULL, NULL);
	child_windows[CWI_BUTTON_WORK_BINARY_TOGGLE] = CreateWindowExW(0, L"BUTTON", L"Work binary", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 82, 121, 132, 22, internal.window, NULL, NULL, NULL);

	for (int i = CWI_BUTTON_SHOW_ENDBOX_TOGGLE; i <= CWI_BUTTON_RECTANGLE; i++)
	{
		SendMessageW(child_windows[i], WM_SETFONT, (WPARAM)internal.font_text, (LPARAM)FALSE);
	}

	child_windows[CWI_TREEVIEW] = CreateWindowExW(0, WC_TREEVIEWW, NULL, WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS, 2, 190, INFO_BOX_CLIENT_WIDTH / 2 - 2, 200, internal.window, NULL, NULL, NULL);
	child_windows[CWI_CURRENT_TREEVIEW] = CreateWindowExW(0, WC_TREEVIEWW, NULL, WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS, INFO_BOX_CLIENT_WIDTH / 2, 190, INFO_BOX_CLIENT_WIDTH / 2 - 2, 200, internal.window, NULL, NULL, NULL);
	SendMessageW(child_windows[CWI_TREEVIEW], WM_SETFONT, (WPARAM)internal.font_text, (LPARAM)FALSE); 
	SendMessageW(child_windows[CWI_CURRENT_TREEVIEW], WM_SETFONT, (WPARAM)internal.font_text, (LPARAM)FALSE);

	campaign_info_show(false);
}

void campaign_info_destroy(void)
{
	for (int i = 0; i < CWI_COUNT; i++)
	{
		DestroyWindow(child_windows[i]);
	}
}

void campaign_info_show(bool is_visible)
{
	for (int i = 0; i < CWI_COUNT; i++)
	{
		ShowWindow(child_windows[i], is_visible ? SW_SHOW : SW_HIDE);
	}
}

bool campaign_info_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, LRESULT* out)
{
	return false;
}

bool campaign_info_handle_interact_tree_item(bool is_global, element_t element)
{
	return false;
}

static void campaign_info_set_internal(const void* unused)
{
	SetWindowTextA(child_windows[CWI_TEXTBOX_TITLE], campaign->name);
}

void campaign_info_set(campaign_t* _campaign, const char* directory)
{
	campaign = _campaign;
	/* i do not like this */
	for (int i = 0; i < 100 && !internal.window; i++)
	{
		Sleep(10);
	}
	RUNTIME_ASSERT(internal.window);
	info_set_caption(path_get_file_name(directory));
	queue_add_from_window(campaign_info_set_internal, NULL);
}

bool campaign_info_find_file(char* directory, size_t size)
{
	OPENFILENAMEA ofn =
	{
		.lStructSize = sizeof ofn,
		.hwndOwner = internal.window,
		.lpstrFilter = "Campaign files\0*.campaign",
		.lpstrFile = directory, .nMaxFile = size,
		.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST
	};
	bool result = !!GetOpenFileNameA(&ofn);
	if (result)
	{
		info_set_caption(path_get_file_name(directory));
	}
	return result;
}