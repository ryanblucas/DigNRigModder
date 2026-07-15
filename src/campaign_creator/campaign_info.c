/*
	campaign_info.c ~ RL
*/

#include "campaign_info.h"
#include "../asset_creator/asset_util.h"
#include "../path.h"
#include "../interface/change_field_modal.h"
#include "../interface/mineral_palette.h"
#include <stdio.h>
#include <Windowsx.h>

enum child_window_index
{
	CWI_LISTBOX_LAYER_FILES,
	CWI_LISTBOX_LAYER_NAMES,

	CWI_TEXTBOX_TITLE,
	CWI_TEXT_TITLE,

	CWI_BUTTON_SHOW_ENDBOX_TOGGLE,

	CWI_BUTTON_ERASER,
	CWI_BUTTON_SELECT,
	CWI_BUTTON_BRUSH,
	CWI_BUTTON_RECTANGLE,
	CWI_BUTTON_MINERAL_ENDBOX,
	CWI_THUMB_BRUSH_SIZE,

	CWI_BUTTON_FILE_SELECTER,
	CWI_TREEVIEW,
	CWI_CURRENT_TREEVIEW,

	CWI_PALETTE,

	CWI_COUNT
};

static LRESULT campaign_info_subclass_edit(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR subclass_id, DWORD_PTR ref_data);

static campaign_t* campaign;
static HWND child_windows[CWI_COUNT];
static info_internal_t internal;

static info_tool_t current_tool;
static asset_block_t asset_palette[3];
static int brush_size;

static asset_t* master_asset;
static asset_t* current_asset;

static region_t region;
static campaign_mode_t mode;
static action_buffer_t action_buffer;

static inline void campaign_set_current_treeview_from_block(asset_info_current_field_t settings)
{
	if (current_tool == TOOL_ERASER || current_tool == TOOL_ENDBOX || current_tool == TOOL_MINERAL_ENDBOX)
	{
		TreeView_DeleteAllItems(child_windows[CWI_CURRENT_TREEVIEW]);
		return;
	}
	asset_set_current_treeview(child_windows[CWI_CURRENT_TREEVIEW], &asset_palette[current_tool], settings);
}

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

	child_windows[CWI_BUTTON_ERASER] = CreateWindowExW(0, L"BUTTON", L"Erase", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 3, 55, 72, 22, internal.window, NULL, NULL, NULL);
	child_windows[CWI_BUTTON_SELECT] = CreateWindowExW(0, L"BUTTON", L"Select", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 3, 78, 72, 22, internal.window, NULL, NULL, NULL);
	child_windows[CWI_BUTTON_BRUSH] = CreateWindowExW(0, L"BUTTON", L"Brush", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 3, 101, 72, 22, internal.window, NULL, NULL, NULL);
	child_windows[CWI_THUMB_BRUSH_SIZE] = CreateWindowExW(0, TRACKBAR_CLASSW, L"Brush size", WS_VISIBLE | WS_CHILD | TBS_AUTOTICKS | TBS_ENABLESELRANGE, 3, 124, 72, 22, internal.window, NULL, NULL, NULL);
	SendMessageW(child_windows[CWI_THUMB_BRUSH_SIZE], TBM_SETRANGE, TRUE, MAKELONG(INFO_BRUSH_MIN_SIZE, INFO_BRUSH_MAX_SIZE));
	child_windows[CWI_BUTTON_RECTANGLE] = CreateWindowExW(0, L"BUTTON", L"End box", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 3, 145, 72, 22, internal.window, NULL, NULL, NULL);
	child_windows[CWI_BUTTON_MINERAL_ENDBOX] = CreateWindowExW(0, L"BUTTON", L"Mineral box", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 3, 168, 72, 22, internal.window, NULL, NULL, NULL);

	child_windows[CWI_BUTTON_SHOW_ENDBOX_TOGGLE] = CreateWindowExW(0, L"BUTTON", L"Show start and end", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 82, 55, 132, 22, internal.window, NULL, NULL, NULL);

	for (int i = CWI_BUTTON_SHOW_ENDBOX_TOGGLE; i <= CWI_BUTTON_MINERAL_ENDBOX; i++)
	{
		SendMessageW(child_windows[i], WM_SETFONT, (WPARAM)internal.font_text, (LPARAM)FALSE);
	}

	child_windows[CWI_TREEVIEW] = CreateWindowExW(0, WC_TREEVIEWW, NULL, WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS, 2, 190, INFO_BOX_CLIENT_WIDTH / 2 - 2, 200, internal.window, NULL, NULL, NULL);
	child_windows[CWI_CURRENT_TREEVIEW] = CreateWindowExW(0, WC_TREEVIEWW, NULL, WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS, INFO_BOX_CLIENT_WIDTH / 2, 190, INFO_BOX_CLIENT_WIDTH / 2 - 2, 200, internal.window, NULL, NULL, NULL);
	SendMessageW(child_windows[CWI_TREEVIEW], WM_SETFONT, (WPARAM)internal.font_text, (LPARAM)FALSE); 
	SendMessageW(child_windows[CWI_CURRENT_TREEVIEW], WM_SETFONT, (WPARAM)internal.font_text, (LPARAM)FALSE);

	mineral_palette_initialize();
	child_windows[CWI_PALETTE] = CreateWindowExW(0, MINERAL_PALETTE_CLASS_NAME, NULL, WS_VISIBLE | WS_CHILD, 84, 124, 32 * 4, 32 * 2, internal.window, NULL, NULL, NULL);
	MINERAL_PALETTE_SET_CELL_SIZE(child_windows[CWI_PALETTE], 4);

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
	mineral_palette_destroy();
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
			int res = MessageBoxW(internal.window, L"That file doesn't exist. Do you want to create it and copy this current layer to it?", L"Error", MB_ICONERROR | MB_YESNO);
			if (res == IDYES)
			{
				file_asset_save(buf, campaign_get_layer(index));
			}
			else if (res == IDNO)
			{
				campaign_info_handle_double_click_listbox(list_box, index);
				return;
			}
		}
		strncpy(campaign->layers[index].directory, buf, MAX_PATH);
		queue_add(internal.events->file_handler, CAMPAIGN_CONVERT_FILE_CHANGE_PARAM_INFO(index));
	}
	else
	{
		strncpy(campaign->layers[index].name, buf, sizeof campaign->layers[index].name);
	}
	ListBox_DeleteString(list_box, index);
	SendMessageA(list_box, LB_INSERTSTRING, (WPARAM)index, (LPARAM)buf);
}

static bool campaign_info_handle_toggle_button(HWND wnd, const char* show, const char* hide, campaign_property_id_t show_id, campaign_property_id_t hide_id)
{
	char buf[20];
	GetWindowTextA(wnd, buf, sizeof buf);
	/* this is good enough */
	if (strncmp(buf, show, sizeof buf) == 0)
	{
		SetWindowTextA(wnd, hide);
		queue_add(internal.events->custom_event_handler, (const void*)show_id);
		return true;
	}
	else if (strncmp(buf, hide, sizeof buf) == 0)
	{
		SetWindowTextA(wnd, show);
		queue_add(internal.events->custom_event_handler, (const void*)hide_id);
		return false;
	}
	RUNTIME_ASSERT(false);
	return false;
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
	else if (wnd == child_windows[CWI_PALETTE] && HIWORD(wparam) == MINERAL_PALETTE_CONTROL_SET_SELECTED_CELL)
	{
		int index = MINERAL_PALETTE_GET_SELECTED_CELL(child_windows[CWI_PALETTE]);
		if (index != -1)
		{
			asset_block_t block;
			MINERAL_PALETTE_GET_CELL(child_windows[CWI_PALETTE], index, &block);
			asset_palette[TOOL_BRUSH] = block;
			if (current_tool == TOOL_BRUSH)
			{
				campaign_set_current_treeview_from_block(0);
			}
		}
	}
	else if (wnd == child_windows[CWI_BUTTON_FILE_SELECTER])
	{
		char path[MAX_PATH];
		path_find_dnr_main(path, sizeof path, NULL);
		if (!campaign_info_find_file(path, sizeof path))
		{
			return;
		}
		queue_add(internal.events->file_handler, queue_copy_data(path, sizeof path));
	}
	else if (wnd == child_windows[CWI_BUTTON_SHOW_ENDBOX_TOGGLE])
	{
		campaign_info_handle_toggle_button(wnd, "Show start and end", "Hide start and end", CPI_ENABLE_END_BOX, CPI_DISABLE_END_BOX);
	}
	else if (index >= CWI_BUTTON_ERASER && index <= CWI_BUTTON_MINERAL_ENDBOX)
	{
		/* not perfect but its good enough */
		if (index == CWI_BUTTON_MINERAL_ENDBOX || index == CWI_BUTTON_RECTANGLE)
		{
			queue_add(internal.events->custom_event_handler, (const void*)CPI_ENABLE_END_BOX);
		}
		if (current_tool == TOOL_ENDBOX || current_tool == TOOL_MINERAL_ENDBOX)
		{
			queue_add(internal.events->custom_event_handler, (const void*)CPI_DISABLE_END_BOX);
		}
		EnableWindow(child_windows[CWI_BUTTON_ERASER + current_tool], TRUE);
		current_tool = index - CWI_BUTTON_ERASER;
		queue_add(internal.events->tool_handler, &current_tool);
		EnableWindow(child_windows[CWI_BUTTON_ERASER + current_tool], FALSE);
		TreeView_DeleteAllItems(child_windows[CWI_CURRENT_TREEVIEW]);
		campaign_set_current_treeview_from_block(0);
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
	case WM_HSCROLL:
	{
		if ((HWND)lparam != child_windows[CWI_THUMB_BRUSH_SIZE] || LOWORD(wparam) != SB_ENDSCROLL)
		{
			return true;
		}
		brush_size = (int)SendMessageW(child_windows[CWI_THUMB_BRUSH_SIZE], TBM_GETPOS, 0, 0);
		queue_add(internal.events->brush_size_handler, &brush_size);
		return true;
	}
	}

	return false;
}

bool campaign_info_handle_interact_tree_item(bool is_global, element_t element)
{
	if (campaign_can_change_field(serialize_element_get_value(element)))
	{
		asset_info_suite_t suite =
		{
			.internal = internal,
			.asset = master_asset,
			.action_buffer = action_buffer,
			.current_tool = current_tool,
			.palette_window = child_windows[CWI_PALETTE],
			.tool_blocks = asset_palette,
			.selection_region = region
		};
		return asset_handle_interact_treeview(&suite, is_global, element);
	}
	return false;
}

action_buffer_t campaign_info_action_buffer_get(void)
{
	return action_buffer;
}

void campaign_info_action_buffer_set(action_buffer_t _action_buffer)
{
	action_buffer = _action_buffer;
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
	TreeView_DeleteAllItems(child_windows[CWI_TREEVIEW]);
	TreeView_DeleteAllItems(child_windows[CWI_CURRENT_TREEVIEW]);

	SetWindowTextA(child_windows[CWI_TEXTBOX_TITLE], campaign->name);

	ListBox_ResetContent(child_windows[CWI_LISTBOX_LAYER_FILES]);
	ListBox_ResetContent(child_windows[CWI_LISTBOX_LAYER_NAMES]);
	for (int i = 0; i < 14; i++)
	{
		SendMessageA(child_windows[CWI_LISTBOX_LAYER_FILES], LB_ADDSTRING, 0, (LPARAM)campaign->layers[i].directory);
		SendMessageA(child_windows[CWI_LISTBOX_LAYER_NAMES], LB_ADDSTRING, 0, (LPARAM)campaign->layers[i].name);
	}
}

void campaign_info_set(campaign_t* _campaign, asset_t* master, const char* directory)
{
	campaign = _campaign;
	master_asset = master;
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

void campaign_info_get_current_brush_block(asset_block_t* res)
{
	*res = asset_palette[TOOL_BRUSH];
}

int campaign_info_get_current_brush_size(void)
{
	return brush_size;
}

info_tool_t campaign_info_get_tool(void)
{
	return current_tool;
}

campaign_mode_t campaign_mode(void)
{
	return mode;
}

void campaign_set_current_layer(asset_t* asset)
{
	current_asset = asset;
	TreeView_DeleteAllItems(child_windows[CWI_TREEVIEW]);
	if (!current_asset)
	{
		return;
	}
#define ADD_SERIALIZABLE(type, name) serialize_single(#type, &current_asset->name, #name, child_windows[CWI_TREEVIEW], NULL);
#define ADD_SERIALIZABLE_ARRAY(type, name, count) serialize_array(#type, &current_asset->name, count, #name, child_windows[CWI_TREEVIEW], NULL);
	SERIALIZABLE_ASSET
#undef ADD_SERIALIZABLE
#undef ADD_SERIALIZABLE_ARRAY
}

void campaign_set_current_region(region_t _region)
{
	region = region_is_invalid(_region) ? _region : region_validate(_region);
	asset_set_current_treeview_from_region(child_windows[CWI_CURRENT_TREEVIEW], master_asset, region, &asset_palette[current_tool]);
}

void campaign_info_palette_save(asset_block_t* palette, size_t palette_size)
{
	int min = min((int)palette_size, MINERAL_PALETTE_GET_SIZE(child_windows[CWI_PALETTE]));
	for (int i = 0; i < min; i++)
	{
		MINERAL_PALETTE_GET_CELL(child_windows[CWI_PALETTE], i, &palette[i]);
	}
}

void campaign_info_palette_copy(const asset_block_t* palette, size_t palette_size)
{
	int min = min((int)palette_size, MINERAL_PALETTE_GET_SIZE(child_windows[CWI_PALETTE]));
	for (int i = 0; i < min; i++)
	{
		MINERAL_PALETTE_SET_CELL(child_windows[CWI_PALETTE], i, &palette[i]);
	}
}