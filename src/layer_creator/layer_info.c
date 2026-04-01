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
	CWI_COUNT
};

static HWND child_windows[CWI_COUNT];
static info_internal_t internal;
static asset_t* asset;

void layer_info_initialize(const info_internal_t* _internal)
{
	internal = *_internal;

	child_windows[CWI_LAYER_FILE_COMBOBOX] = CreateWindowExW(0, WC_COMBOBOXW, NULL, WS_VSCROLL | CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE, 40, 25, 200, 200, internal.window, NULL, NULL, NULL);
	SendMessageW(child_windows[CWI_LAYER_FILE_COMBOBOX], WM_SETFONT, (WPARAM)internal.font_caption, 0);

	child_windows[CWI_LAYER_FILE_COMBOBOX_LABEL] = CreateWindowExW(0, L"STATIC", L"File:", WS_VISIBLE | WS_CHILD, 2, 25, 36, 18, internal.window, NULL, NULL, NULL);
	SendMessageW(child_windows[CWI_LAYER_FILE_COMBOBOX_LABEL], WM_SETFONT, (WPARAM)internal.font_caption, 0);

	child_windows[CWI_LAYER_TREEVIEW] = CreateWindowExW(0, WC_TREEVIEWW, NULL, WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS, 2, 190, INFO_BOX_CLIENT_WIDTH / 2 - 2, 200, internal.window, NULL, NULL, NULL);
	SendMessageW(child_windows[CWI_LAYER_TREEVIEW], WM_SETFONT, (WPARAM)internal.font_text, (LPARAM)FALSE);

	child_windows[CWI_LAYER_CURRENT_TREEVIEW] = CreateWindowExW(0, WC_TREEVIEWW, NULL, WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS, INFO_BOX_CLIENT_WIDTH / 2, 190, INFO_BOX_CLIENT_WIDTH / 2 - 2, 200, internal.window, NULL, NULL, NULL);
	SendMessageW(child_windows[CWI_LAYER_CURRENT_TREEVIEW], WM_SETFONT, (WPARAM)internal.font_text, (LPARAM)FALSE);

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

bool layer_info_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, LRESULT* out)
{
	*out = 0;
	switch (msg)
	{
	case WM_COMMAND:
	{
		if ((HWND)lparam == child_windows[CWI_LAYER_FILE_COMBOBOX])
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
		break;
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

enum layer_info_current_field
{
	LICF_TILE_TYPE = 1 << 0,
	LICF_VISUAL = 1 << 1,
	LICF_TRANSPARENCY = 1 << 2
};

void layer_info_asset_set_current(region_t region)
{
	RUNTIME_ASSERT(!region_is_invalid(region));
	region = region_validate(region);
	TreeView_DeleteAllItems(child_windows[CWI_LAYER_CURRENT_TREEVIEW]);
	asset_block_t common = asset->blocks[region.x0 + region.y0 * TARGET_WIDTH];
	enum layer_info_current_field mask = 0;
	for (int y = 0; y < region_height(region); y++)
	{
		for (int x = 0; x < region_width(region); x++)
		{
			asset_block_t* curr = &asset->blocks[(region.x0 + x) + (region.y0 + y) * TARGET_WIDTH];
			if (~mask & LICF_TILE_TYPE && common.tile_type != curr->tile_type)
			{
				common.tile_type = 0;
				mask |= LICF_TILE_TYPE;
			}
			else if (~mask & LICF_VISUAL && common.visual.Attributes != curr->visual.Attributes || common.visual.Char.AsciiChar != curr->visual.Char.AsciiChar)
			{
				common.visual = (CHAR_INFO){ 0 };
				mask |= LICF_VISUAL;
			}
			else if (~mask & LICF_TILE_TYPE && common.transparency != curr->transparency)
			{
				common.transparency = false;
				mask |= LICF_TRANSPARENCY;
			}
		}
	}

#define ADD_SERIALIZABLE(type, name) element_t name = serialize_single(#type, &common.name, #name, child_windows[CWI_LAYER_CURRENT_TREEVIEW], NULL);
#define ADD_SERIALIZABLE_ARRAY(type, name, count) element_t name = serialize_array(#type, &common.name, count, #name, child_windows[CWI_LAYER_CURRENT_TREEVIEW], NULL);
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