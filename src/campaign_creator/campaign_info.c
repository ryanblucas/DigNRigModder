/*
	campaign_info.c ~ RL
*/

#include "campaign_info.h"
#include "../path.h"
#include "../interface/change_field_modal.h"
#include <stdio.h>
#include <Windowsx.h>

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
	CWI_BUTTON_RECTANGLE,
	CWI_THUMB_BRUSH_SIZE,

	CWI_BUTTON_FILE_SELECTER,
	CWI_TREEVIEW,
	CWI_CURRENT_TREEVIEW,

	CWI_COUNT
};

static LRESULT campaign_info_subclass_edit(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR subclass_id, DWORD_PTR ref_data);

static campaign_t* campaign;
static HWND child_windows[CWI_COUNT];
static info_internal_t internal;

static info_tool_t current_tool;

void campaign_info_initialize(info_internal_t* _internal)
{
	internal = *_internal;

	child_windows[CWI_TEXT_TITLE] = CreateWindowExW(0, L"STATIC", L"Title: ", WS_CHILD | WS_OVERLAPPED | WS_VISIBLE, 82, 33, 30, 14, internal.window, NULL, NULL, NULL);
	child_windows[CWI_TEXTBOX_TITLE] = CreateWindowExW(0, L"EDIT", NULL, WS_CHILD | WS_OVERLAPPED | WS_VISIBLE | WS_BORDER, 112, 29, 102, 22, internal.window, NULL, NULL, NULL);
	SendMessageW(child_windows[CWI_TEXT_TITLE], WM_SETFONT, (WPARAM)internal.font_caption, 0);
	SendMessageW(child_windows[CWI_TEXTBOX_TITLE], WM_SETFONT, (WPARAM)internal.font_caption, 0);
	SetWindowSubclass(child_windows[CWI_TEXTBOX_TITLE], campaign_info_subclass_edit, 0, 0);

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

	_internal->global_treeview = child_windows[CWI_TREEVIEW];
	_internal->current_treeview = child_windows[CWI_CURRENT_TREEVIEW];

	current_tool = TOOL_SELECT;
	EnableWindow(child_windows[CWI_BUTTON_SELECT], FALSE);

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

static void campaign_info_handle_double_click_listbox(HWND list_box, int index)
{
	char buf[MAX_PATH];
	RUNTIME_ASSERT(ListBox_GetTextLen(list_box, index) < sizeof buf);
	SendMessageA(list_box, LB_GETTEXT, (WPARAM)index, (LPARAM)buf);
	change_field_modal_string(internal.window, buf, sizeof buf);
	if (!*buf)
	{
		return;
	}
	if (list_box == child_windows[CWI_LISTBOX_LAYER_FILES])
	{
		char layer_path_buf[MAX_PATH];
		path_find_dnr_main_chain(layer_path_buf, sizeof layer_path_buf, "Layers", buf);
		if (!path_exists(layer_path_buf))
		{
			MessageBeep(MB_ICONERROR);
			if (MessageBoxW(internal.window, L"That file doesn't exist.", L"Error", MB_ICONERROR | MB_OKCANCEL) != IDCANCEL)
			{
				campaign_info_handle_double_click_listbox(list_box, index);
				return;
			}
		}
		strncpy(campaign->layers[index].directory, buf, MAX_PATH);
		queue_add(internal.events->file_handler, (const void*)index);
	}
	else
	{
		strncpy(campaign->layers[index].name, buf, sizeof campaign->layers[index].name);
	}
	ListBox_DeleteString(list_box, index);
	SendMessageA(list_box, LB_INSERTSTRING, (WPARAM)index, (LPARAM)buf);
}

static void campaign_info_handle_toggle_button(HWND wnd, const char* show, const char* hide, campaign_property_id_t show_id, campaign_property_id_t hide_id)
{
	char buf[20];
	GetWindowTextA(wnd, buf, sizeof buf);
	/* this is good enough */
	if (strncmp(buf, show, sizeof buf) == 0)
	{
		SetWindowTextA(wnd, hide);
		queue_add(internal.events->custom_event_handler, (const void*)show_id);
	}
	else if (strncmp(buf, hide, sizeof buf) == 0)
	{
		SetWindowTextA(wnd, show);
		queue_add(internal.events->custom_event_handler, (const void*)hide_id);
	}
	else
	{
		RUNTIME_ASSERT(false);
	}
}

static enum child_window_index campaign_info_child_window_from_handle(HWND hwnd)
{
	for (int i = 0; i < CWI_COUNT; i++)
	{
		if (child_windows[i] == hwnd)
		{
			return i;
		}
	}
	return -1;
}

static void campaign_info_dispatch_command(WPARAM wparam, LPARAM lparam)
{
	HWND wnd = (HWND)lparam;
	int index = campaign_info_child_window_from_handle(wnd);
	if (HIWORD(wparam) == LBN_DBLCLK)
	{
		int selected_index = ListBox_GetCurSel(wnd);
		if (selected_index != LB_ERR)
		{
			campaign_info_handle_double_click_listbox(wnd, selected_index);
		}
	}
	else if (HIWORD(wparam) == LBN_SELCHANGE)
	{
		int selected_index = ListBox_GetCurSel(wnd);
		if (selected_index == LB_ERR)
		{
			return;
		}
		ListBox_SetCurSel(child_windows[!index], selected_index);
	}
	else if (HIWORD(wparam) == EN_CHANGE && wnd == child_windows[CWI_TEXTBOX_TITLE])
	{
		GetWindowTextA(wnd, campaign->name, sizeof campaign->name);
	}
	else if (wnd == child_windows[CWI_BUTTON_SHOW_BINARY_TOGGLE])
	{
		campaign_info_handle_toggle_button(wnd, "Show binary", "Hide binary", CPI_ENABLE_SHOW_BINARY_MODE, CPI_DISABLE_SHOW_BINARY_MODE);
	}
	else if (wnd == child_windows[CWI_BUTTON_SHOW_ENDBOX_TOGGLE])
	{
		campaign_info_handle_toggle_button(wnd, "Show endbox", "Hide endbox", CPI_ENABLE_END_BOX, CPI_DISABLE_END_BOX);
	}
	else if (wnd == child_windows[CWI_BUTTON_WORK_BINARY_TOGGLE])
	{
		campaign_info_handle_toggle_button(wnd, "Work binary", "Work asset", CPI_CHANGE_TO_BINARY_MODE, CPI_CHANGE_TO_ASSET_MODE);
	}
	else if (index >= CWI_BUTTON_ERASER && index <= CWI_BUTTON_RECTANGLE)
	{
		EnableWindow(child_windows[CWI_BUTTON_ERASER + current_tool], TRUE);
		current_tool = index - CWI_BUTTON_ERASER;
		queue_add(internal.events->tool_handler, &current_tool);
		EnableWindow(child_windows[CWI_BUTTON_ERASER + current_tool], FALSE);
	}
}

bool campaign_info_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, LRESULT* out)
{
	*out = 0;
	switch (msg)
	{
	case WM_COMMAND:
	{
		campaign_info_dispatch_command(wparam, lparam);
		return true;
	}
	}

	return false;
}

bool campaign_info_handle_interact_tree_item(bool is_global, element_t element)
{
	return false;
}

static LRESULT campaign_info_subclass_edit(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR subclass_id, DWORD_PTR ref_data)
{
	switch (msg)
	{
	case WM_LBUTTONDOWN:
	{
		if (GetFocus() == hwnd)
		{
			HWND parent = GetParent(hwnd);
			SetFocus(parent);
			return 0;
		}
		break;
	}
	case WM_NCDESTROY:
		RemoveWindowSubclass(hwnd, campaign_info_subclass_edit, subclass_id);
		break;
	}
	return DefSubclassProc(hwnd, msg, wparam, lparam);
}

static void campaign_info_set_internal(const void* unused)
{
	SetWindowTextA(child_windows[CWI_TEXTBOX_TITLE], campaign->name);
	for (int i = 0; i < 14; i++)
	{
		SendMessageA(child_windows[CWI_LISTBOX_LAYER_FILES], LB_ADDSTRING, 0, (LPARAM)campaign->layers[i].directory);
		SendMessageA(child_windows[CWI_LISTBOX_LAYER_NAMES], LB_ADDSTRING, 0, (LPARAM)campaign->layers[i].name);
	}
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
		.lpstrFile = directory, .nMaxFile = (DWORD)size,
		.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST
	};
	bool result = !!GetOpenFileNameA(&ofn);
	if (result)
	{
		info_set_caption(path_get_file_name(directory));
	}
	return result;
}

info_tool_t campaign_info_get_tool(void)
{
	return current_tool;
}