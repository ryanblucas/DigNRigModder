/*
	layer_info.c ~ RL
*/

#include "layer_info.h"
#include "../path.h"

#define INFO_BOX_MSG_STATE_READY (WM_USER + 0x20)

enum child_window_index
{
	CWI_LAYER_FILE_COMBOBOX,
	CWI_LAYER_FILE_COMBOBOX_LABEL,
	CWI_LAYER_TREEVIEW,
	CWI_LAYER_CURRENT_TREEVIEW,
	CWI_LAYER_ERASER_BUTTON,
	CWI_LAYER_SELECT_BUTTON,
	CWI_LAYER_BRUSH_BUTTON,
	CWI_LAYER_BRUSH_SIZE_THUMB,
	CWI_COUNT
};

static HWND child_windows[CWI_COUNT];
static info_internal_t internal;
static asset_t* asset;

static info_tool_t current_tool;

static asset_block_t blocks[3];
static region_t region;

static action_buffer_t action_buffer;

static int brush_size = 1;

enum layer_info_current_field
{
	LICF_TILE_TYPE = 1 << 0,
	LICF_VISUAL = 1 << 1,
	LICF_TRANSPARENCY = 1 << 2
};

static inline void layer_info_asset_set_treeview(enum layer_info_current_field mask)
{
	TreeView_DeleteAllItems(child_windows[CWI_LAYER_CURRENT_TREEVIEW]);
#define ADD_SERIALIZABLE(type, name) element_t name = serialize_single(#type, &blocks[current_tool].name, #name, child_windows[CWI_LAYER_CURRENT_TREEVIEW], NULL);
#define ADD_SERIALIZABLE_ARRAY(type, name, count) element_t name = serialize_array(#type, &blocks[current_tool].name, count, #name, child_windows[CWI_LAYER_CURRENT_TREEVIEW], NULL);
	SERIALIZABLE_ASSET_BLOCK
#undef ADD_SERIALIZABLE
#undef ADD_SERIALIZABLE_ARRAY

	if (mask & LICF_TILE_TYPE)
	{
		serialize_element_enable(tile_type, false);
	}
	if (mask & LICF_VISUAL)
	{
		serialize_element_enable(visual, false);
	}
	if (mask & LICF_TRANSPARENCY)
	{
		serialize_element_enable(transparency, false);
	}
}

void layer_info_initialize(info_internal_t* _internal)
{
	internal = *_internal;

	child_windows[CWI_LAYER_FILE_COMBOBOX] = CreateWindowExW(0, WC_COMBOBOXW, NULL, WS_VSCROLL | CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE, 40, 25, INFO_BOX_CLIENT_WIDTH / 2 - 40, 200, internal.window, NULL, NULL, NULL);
	SendMessageW(child_windows[CWI_LAYER_FILE_COMBOBOX], WM_SETFONT, (WPARAM)internal.font_caption, 0);

	child_windows[CWI_LAYER_FILE_COMBOBOX_LABEL] = CreateWindowExW(0, L"STATIC", L"File:", WS_VISIBLE | WS_CHILD, 8, 29, 24, 18, internal.window, NULL, NULL, NULL);
	SendMessageW(child_windows[CWI_LAYER_FILE_COMBOBOX_LABEL], WM_SETFONT, (WPARAM)internal.font_caption, 0);

	child_windows[CWI_LAYER_TREEVIEW] = CreateWindowExW(0, WC_TREEVIEWW, NULL, WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS, 2, 190, INFO_BOX_CLIENT_WIDTH / 2 - 2, 200, internal.window, NULL, NULL, NULL);
	SendMessageW(child_windows[CWI_LAYER_TREEVIEW], WM_SETFONT, (WPARAM)internal.font_text, (LPARAM)FALSE);

	child_windows[CWI_LAYER_CURRENT_TREEVIEW] = CreateWindowExW(0, WC_TREEVIEWW, NULL, WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS, INFO_BOX_CLIENT_WIDTH / 2, 190, INFO_BOX_CLIENT_WIDTH / 2 - 2, 200, internal.window, NULL, NULL, NULL);
	SendMessageW(child_windows[CWI_LAYER_CURRENT_TREEVIEW], WM_SETFONT, (WPARAM)internal.font_text, (LPARAM)FALSE);

	child_windows[CWI_LAYER_ERASER_BUTTON] = CreateWindowExW(0, L"BUTTON", L"Erase", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 3, 98, 72, 22, internal.window, NULL, NULL, NULL);
	child_windows[CWI_LAYER_SELECT_BUTTON] = CreateWindowExW(0, L"BUTTON", L"Select", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 3, 121, 72, 22, internal.window, NULL, NULL, NULL);
	child_windows[CWI_LAYER_BRUSH_BUTTON] = CreateWindowExW(0, L"BUTTON", L"Brush", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 3, 144, 72, 22, internal.window, NULL, NULL, NULL);
	child_windows[CWI_LAYER_BRUSH_SIZE_THUMB] = CreateWindowExW(0, TRACKBAR_CLASSW, L"Brush size", WS_VISIBLE | WS_CHILD | TBS_AUTOTICKS | TBS_ENABLESELRANGE, 3, 167, 72, 22, internal.window, NULL, NULL, NULL);
	SendMessageW(child_windows[CWI_LAYER_BRUSH_SIZE_THUMB], TBM_SETRANGE, TRUE, MAKELONG(INFO_BRUSH_MIN_SIZE, INFO_BRUSH_MAX_SIZE));

	for (int i = 0; i < 3; i++)
	{
		SendMessageW(child_windows[CWI_LAYER_ERASER_BUTTON + i], WM_SETFONT, (WPARAM)internal.font_text, (LPARAM)FALSE);
	}

	_internal->global_treeview = child_windows[CWI_LAYER_TREEVIEW];
	_internal->current_treeview = child_windows[CWI_LAYER_CURRENT_TREEVIEW];

	current_tool = TOOL_SELECT;
	EnableWindow(child_windows[CWI_LAYER_SELECT_BUTTON], FALSE);
	layer_info_show(false);
}

void layer_info_destroy(void)
{
	for (int i = 0; i < CWI_COUNT; i++)
	{
		DestroyWindow(child_windows[i]);
	}
}

static void layer_info_populate_combobox(int max)
{
	debug_profiler_push();
	RUNTIME_ASSERT(SendMessageW(child_windows[CWI_LAYER_FILE_COMBOBOX], CB_RESETCONTENT, 0, 0) != CB_ERR);
	char buf[MAX_PATH];
	char* directories = path_enumerate_directory_create(path_find_dnr_main(buf, sizeof buf, "Layers\\*.layer"), &max);
	char* curr = directories;
	while (max-- > 0)
	{
		/* more research on if this is a valid move here to do SendMessageA instead of converting the string to a wide one then SendMessageW */
		RUNTIME_ASSERT(SendMessageA(child_windows[CWI_LAYER_FILE_COMBOBOX], CB_ADDSTRING, 0, (LPARAM)curr) != CB_ERR);
		curr += strnlen(curr, MAX_PATH) + 1;
	}
	free(directories);
	debug_profiler_pop("Populating layer combobox");
}

static void layer_info_sync_combobox_with_console(void)
{
	char buf[MAX_PATH];
	path_find_dnr_main(buf, sizeof buf, "Layers\\");
	char* end = buf + strnlen(buf, sizeof buf);
	GetWindowTextA(child_windows[CWI_LAYER_FILE_COMBOBOX], end, (int)(sizeof buf - (size_t)(end - buf)));
	RAISE_EVENT(internal.events->file_handler, buf);
}

void layer_info_show(bool is_visible)
{
	if (is_visible)
	{
		layer_info_populate_combobox(1);
		SendMessageW(child_windows[CWI_LAYER_FILE_COMBOBOX], CB_SETCURSEL, (WPARAM)0, 0);
		layer_info_sync_combobox_with_console();
	}

	for (int i = 0; i < CWI_COUNT; i++)
	{
		ShowWindow(child_windows[i], is_visible ? SW_SHOW : SW_HIDE);
	}
}

static void layer_info_state_set_tree_view(asset_t* asset)
{
	debug_profiler_push();
	asset_t* item = asset;

#define ADD_SERIALIZABLE(type, name) serialize_single(#type, &item->name, #name, child_windows[CWI_LAYER_TREEVIEW], NULL);
#define ADD_SERIALIZABLE_ARRAY(type, name, count) serialize_array(#type, &item->name, count, #name, child_windows[CWI_LAYER_TREEVIEW], NULL);
	SERIALIZABLE_ASSET
#undef ADD_SERIALIZABLE
#undef ADD_SERIALIZABLE_ARRAY

	debug_profiler_pop("Surface level serializing");
}

static enum child_window_index layer_info_child_window_from_handle(HWND hwnd)
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

bool layer_info_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, LRESULT* out)
{
	*out = 0;
	switch (msg)
	{
	case WM_COMMAND:
	{
		HWND window = (HWND)lparam;
		enum child_window_index index = layer_info_child_window_from_handle(window);
		if (window == child_windows[CWI_LAYER_FILE_COMBOBOX])
		{
			if (HIWORD(wparam) == CBN_DROPDOWN)
			{
				/* refresh every time expanded.. good idea? or waste of time? */
				layer_info_populate_combobox(0);
			}
			else if (HIWORD(wparam) == CBN_SELCHANGE)
			{
				layer_info_sync_combobox_with_console();
			}
		}
		else if (index >= CWI_LAYER_ERASER_BUTTON && index <= CWI_LAYER_BRUSH_BUTTON)
		{
			TreeView_DeleteAllItems(child_windows[CWI_LAYER_CURRENT_TREEVIEW]);
			EnableWindow(child_windows[CWI_LAYER_ERASER_BUTTON + current_tool], TRUE);
			current_tool = index - CWI_LAYER_ERASER_BUTTON;
			region = INVALID_REGION;
			if (current_tool != TOOL_ERASER)
			{
				layer_info_asset_set_treeview(0);
			}
			RAISE_EVENT(internal.events->tool_handler, current_tool);
			EnableWindow(child_windows[CWI_LAYER_ERASER_BUTTON + current_tool], FALSE);
		}
		break;
	}
	case WM_HSCROLL:
	{
		if ((HWND)lparam != child_windows[CWI_LAYER_BRUSH_SIZE_THUMB] || LOWORD(wparam) != SB_ENDSCROLL)
		{
			return true;
		}
		brush_size = (int)SendMessageW(child_windows[CWI_LAYER_BRUSH_SIZE_THUMB], TBM_GETPOS, 0, 0);
		RAISE_EVENT(internal.events->brush_size_handler, brush_size);
		return true;
	}
	case INFO_BOX_MSG_STATE_READY:
	{
		TreeView_DeleteAllItems(child_windows[CWI_LAYER_TREEVIEW]);
		TreeView_DeleteAllItems(child_windows[CWI_LAYER_CURRENT_TREEVIEW]);
		layer_info_state_set_tree_view((asset_t*)wparam);
		break;
	}
	}
	return false;
}

void layer_info_handle_interact_tree_item(bool is_global, element_t element)
{
	/* only should change elementary fields */
	if (serialize_element_get_size(element) > 4 || serialize_element_get_count(element) > 1)
	{
		return;
	}

	if (is_global)
	{
		field_t begin_copy = field_create(serialize_element_get_value(element), serialize_element_get_size(element));
		if (!serialize_on_change_field(element) || begin_copy == field_create(serialize_element_get_value(element), serialize_element_get_size(element)))
		{
			return;
		}
		RAISE_EVENT(internal.events->global_field_handler, serialize_element_get_value(element));
		action_buffer_add_field(action_buffer, element, begin_copy);
		return;
	}

	field_t previous = field_create(serialize_element_get_value(element), serialize_element_get_size(element));
	if (!serialize_on_change_field(element) || previous == field_create(serialize_element_get_value(element), serialize_element_get_size(element)))
	{
		return;
	}
	if (region_is_invalid(region))
	{
		return;
	}
	action_buffer_pre_add_asset_block(action_buffer, asset, region);
	for (int y = region.y0; y <= region.y1; y++)
	{
		for (int x = region.x0; x <= region.x1; x++)
		{
			uint8_t* current = (uint8_t*)serialize_element_get_value(element);
			memcpy((uint8_t*)&asset->blocks[x + y * TARGET_WIDTH] + (current - (uint8_t*)&blocks[current_tool]), current, serialize_element_get_size(element));
		}
	}
	RAISE_EVENT(internal.events->block_handler, region);
	action_buffer_post_add_asset_block(action_buffer, asset);
}

void layer_info_asset_set(asset_t* _asset)
{
	asset = _asset;
	/* i do not like this */
	for (int i = 0; i < 100 && !internal.window; i++)
	{
		Sleep(10);
	}
	RUNTIME_ASSERT(internal.window);
	PostMessageW(internal.window, INFO_BOX_MSG_STATE_READY, (WPARAM)asset, 0);
}

void layer_info_asset_set_current(region_t _region)
{
	if (region_is_invalid(_region))
	{
		TreeView_DeleteAllItems(child_windows[CWI_LAYER_CURRENT_TREEVIEW]);
		region = _region;
		return;
	}
	region = region_validate(_region);
	blocks[current_tool] = asset->blocks[region.x0 + region.y0 * TARGET_WIDTH];
	enum layer_info_current_field mask = 0;
	for (int y = 0; y < region_height(region); y++)
	{
		for (int x = 0; x < region_width(region); x++)
		{
			asset_block_t* curr = &asset->blocks[(region.x0 + x) + (region.y0 + y) * TARGET_WIDTH];
			if (~mask & LICF_TILE_TYPE && blocks[current_tool].tile_type != curr->tile_type)
			{
				blocks[current_tool].tile_type = 0;
				mask |= LICF_TILE_TYPE;
			}
			else if (~mask & LICF_VISUAL && blocks[current_tool].visual.Attributes != curr->visual.Attributes || blocks[current_tool].visual.Char.AsciiChar != curr->visual.Char.AsciiChar)
			{
				blocks[current_tool].visual = (CHAR_INFO){ 0 };
				mask |= LICF_VISUAL;
			}
			else if (~mask & LICF_TILE_TYPE && blocks[current_tool].transparency != curr->transparency)
			{
				blocks[current_tool].transparency = false;
				mask |= LICF_TRANSPARENCY;
			}
		}
	}

	layer_info_asset_set_treeview(mask);
}

action_buffer_t layer_info_action_buffer_get(void)
{
	return action_buffer;
}

void layer_info_action_buffer_set(action_buffer_t _action_buffer)
{
	action_buffer = _action_buffer;
}

info_tool_t layer_info_get_current_tool(void)
{
	return current_tool;
}

void layer_info_get_current_brush_block(asset_block_t* res)
{
	*res = blocks[TOOL_BRUSH];
}

int layer_info_get_current_brush_size(void)
{
	return brush_size;
}