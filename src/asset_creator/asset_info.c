/*
	asset_info.c ~ RL
*/

#include "asset_info.h"
#include "asset_util.h"
#include "asset_main.h"
#include "../path.h"
#include "../interface/mineral_palette.h"
#include <stdio.h>

#define INFO_BOX_MSG_STATE_READY (WM_USER + 0x20)

enum child_window_index
{
	CWI_ASSET_FILE_SELECTER,

	CWI_ASSET_TREEVIEW,
	CWI_ASSET_CURRENT_TREEVIEW,

	CWI_ASSET_ERASER_BUTTON,
	CWI_ASSET_SELECT_BUTTON,
	CWI_ASSET_BRUSH_BUTTON,
	CWI_ASSET_BRUSH_SIZE_THUMB,

	CWI_ASSET_COPY_SELECTED_BUTTON,

	CWI_ASSET_PALETTE,

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

static char directory[MAX_PATH];

static inline void asset_set_current_treeview_from_block(asset_info_current_field_t settings)
{
	asset_set_current_treeview(child_windows[CWI_ASSET_CURRENT_TREEVIEW], &blocks[current_tool], settings);
}

void asset_info_initialize(info_internal_t* _internal)
{
	internal = *_internal;

	child_windows[CWI_ASSET_FILE_SELECTER] = CreateWindowExW(0, L"BUTTON", NULL, WS_CHILD | WS_OVERLAPPED | WS_VISIBLE, 8, 29, 62, 22, internal.window, NULL, NULL, NULL);
	SendMessageW(child_windows[CWI_ASSET_FILE_SELECTER], WM_SETFONT, (WPARAM)internal.font_caption, 0);
	SetWindowTextW(child_windows[CWI_ASSET_FILE_SELECTER], L"Find file");

	child_windows[CWI_ASSET_TREEVIEW] = CreateWindowExW(0, WC_TREEVIEWW, NULL, WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS, 2, 190, INFO_BOX_CLIENT_WIDTH / 2 - 2, 200, internal.window, NULL, NULL, NULL);
	SendMessageW(child_windows[CWI_ASSET_TREEVIEW], WM_SETFONT, (WPARAM)internal.font_text, (LPARAM)FALSE);

	child_windows[CWI_ASSET_CURRENT_TREEVIEW] = CreateWindowExW(0, WC_TREEVIEWW, NULL, WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS, INFO_BOX_CLIENT_WIDTH / 2, 190, INFO_BOX_CLIENT_WIDTH / 2 - 2, 200, internal.window, NULL, NULL, NULL);
	SendMessageW(child_windows[CWI_ASSET_CURRENT_TREEVIEW], WM_SETFONT, (WPARAM)internal.font_text, (LPARAM)FALSE);

	child_windows[CWI_ASSET_ERASER_BUTTON] = CreateWindowExW(0, L"BUTTON", L"Erase", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 3, 98, 72, 22, internal.window, NULL, NULL, NULL);
	child_windows[CWI_ASSET_SELECT_BUTTON] = CreateWindowExW(0, L"BUTTON", L"Select", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 3, 121, 72, 22, internal.window, NULL, NULL, NULL);
	child_windows[CWI_ASSET_BRUSH_BUTTON] = CreateWindowExW(0, L"BUTTON", L"Brush", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 3, 144, 72, 22, internal.window, NULL, NULL, NULL);
	child_windows[CWI_ASSET_BRUSH_SIZE_THUMB] = CreateWindowExW(0, TRACKBAR_CLASSW, L"Brush size", WS_VISIBLE | WS_CHILD | TBS_AUTOTICKS | TBS_ENABLESELRANGE, 3, 167, 72, 22, internal.window, NULL, NULL, NULL);
	SendMessageW(child_windows[CWI_ASSET_BRUSH_SIZE_THUMB], TBM_SETRANGE, TRUE, MAKELONG(INFO_BRUSH_MIN_SIZE, INFO_BRUSH_MAX_SIZE));
	
	child_windows[CWI_ASSET_COPY_SELECTED_BUTTON] = CreateWindowExW(0, L"BUTTON", L"Copy selected to brush", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 100, 98, 120, 22, internal.window, NULL, NULL, NULL);
	SendMessageW(child_windows[CWI_ASSET_COPY_SELECTED_BUTTON], WM_SETFONT, (WPARAM)internal.font_text, (LPARAM)FALSE);

	for (int i = CWI_ASSET_ERASER_BUTTON; i <= CWI_ASSET_COPY_SELECTED_BUTTON; i++)
	{
		SendMessageW(child_windows[i], WM_SETFONT, (WPARAM)internal.font_text, (LPARAM)FALSE);
	}

	mineral_palette_initialize();
	child_windows[CWI_ASSET_PALETTE] = CreateWindowExW(0, MINERAL_PALETTE_CLASS_NAME, NULL, WS_VISIBLE | WS_CHILD, 300, 50, 48, 24 * 4, internal.window, NULL, NULL, NULL);
	MINERAL_PALETTE_SET_CELL_SIZE(child_windows[CWI_ASSET_PALETTE], 3);

	_internal->global_treeview = child_windows[CWI_ASSET_TREEVIEW];
	_internal->current_treeview = child_windows[CWI_ASSET_CURRENT_TREEVIEW];

	current_tool = TOOL_SELECT;
	EnableWindow(child_windows[CWI_ASSET_SELECT_BUTTON], FALSE);
	asset_info_show(false);
}

void asset_info_destroy(void)
{
	for (int i = 0; i < CWI_COUNT; i++)
	{
		DestroyWindow(child_windows[i]);
	}
	mineral_palette_destroy();
}

void asset_info_show(bool is_visible)
{
	if (is_visible && !*directory)
	{
		char buf[MAX_PATH];
		path_find_dnr_main(buf, sizeof buf, "Layers");
		snprintf(directory, sizeof directory, "%s\\*.layer", buf);

		WIN32_FIND_DATAA wfd;
		HANDLE handle = FindFirstFileA(directory, &wfd);
		RUNTIME_ASSERT(handle != INVALID_HANDLE_VALUE);
		FindClose(handle);

		snprintf(directory, sizeof directory, "%s\\%s", buf, wfd.cFileName);
		info_set_caption(wfd.cFileName);
		queue_add(internal.events->file_handler, directory);
	}

	for (int i = 0; i < CWI_COUNT; i++)
	{
		ShowWindow(child_windows[i], is_visible ? SW_SHOW : SW_HIDE);
	}
}

static void asset_info_state_set_tree_view(asset_t* asset)
{
	debug_profiler_push();
	asset_t* item = asset;

#define ADD_SERIALIZABLE(type, name) serialize_single(#type, &item->name, #name, child_windows[CWI_ASSET_TREEVIEW], NULL);
#define ADD_SERIALIZABLE_ARRAY(type, name, count) serialize_array(#type, &item->name, count, #name, child_windows[CWI_ASSET_TREEVIEW], NULL);
	SERIALIZABLE_ASSET
#undef ADD_SERIALIZABLE
#undef ADD_SERIALIZABLE_ARRAY

	debug_profiler_pop("Surface level serializing");
}

static enum child_window_index asset_info_child_window_from_handle(HWND hwnd)
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

static void asset_info_handle_command(HWND window, WPARAM wparam)
{
	enum child_window_index index = asset_info_child_window_from_handle(window);
	if (window == child_windows[CWI_ASSET_FILE_SELECTER])
	{
		OPENFILENAMEA ofn =
		{
			.lStructSize = sizeof ofn,
			.hwndOwner = internal.window,
			.lpstrFilter = "Asset files\0*.layer;*.sprite",
			.lpstrFile = directory, .nMaxFile = sizeof directory,
			.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST
		};
		GetOpenFileNameA(&ofn);
		info_set_caption(path_get_file_name(directory));
		queue_add(internal.events->file_handler, directory);
	}
	else if (window == child_windows[CWI_ASSET_PALETTE] && HIWORD(wparam) == MINERAL_PALETTE_CONTROL_SET_SELECTED_CELL)
	{
		int index = MINERAL_PALETTE_GET_SELECTED_CELL(child_windows[CWI_ASSET_PALETTE]);
		if (index != -1)
		{
			asset_block_t block;
			MINERAL_PALETTE_GET_CELL(child_windows[CWI_ASSET_PALETTE], index, &block);
			blocks[TOOL_BRUSH] = block;
			if (current_tool == TOOL_BRUSH)
			{
				asset_set_current_treeview_from_block(0);
			}
		}
	}
	else if (index >= CWI_ASSET_ERASER_BUTTON && index <= CWI_ASSET_BRUSH_BUTTON)
	{
		TreeView_DeleteAllItems(child_windows[CWI_ASSET_CURRENT_TREEVIEW]);
		EnableWindow(child_windows[CWI_ASSET_ERASER_BUTTON + current_tool], TRUE);
		current_tool = index - CWI_ASSET_ERASER_BUTTON;
		region = INVALID_REGION;
		if (current_tool != TOOL_ERASER)
		{
			asset_set_current_treeview_from_block(0);
		}
		queue_add(internal.events->tool_handler, &current_tool);
		EnableWindow(child_windows[CWI_ASSET_ERASER_BUTTON + current_tool], FALSE);
	}
	else if (index == CWI_ASSET_COPY_SELECTED_BUTTON)
	{
		blocks[TOOL_BRUSH] = blocks[TOOL_SELECT];
		MINERAL_PALETTE_SET_CELL(child_windows[CWI_ASSET_PALETTE], MINERAL_PALETTE_GET_SELECTED_CELL(child_windows[CWI_ASSET_PALETTE]), &blocks[TOOL_BRUSH]);
		if (current_tool == TOOL_BRUSH)
		{
			asset_set_current_treeview_from_block(0);
		}
	}
}

bool asset_info_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, LRESULT* out)
{
	*out = 0;
	switch (msg)
	{
	case WM_COMMAND:
	{
		asset_info_handle_command((HWND)lparam, wparam);
		break;
	}
	case WM_HSCROLL:
	{
		if ((HWND)lparam != child_windows[CWI_ASSET_BRUSH_SIZE_THUMB] || LOWORD(wparam) != SB_ENDSCROLL)
		{
			return true;
		}
		brush_size = (int)SendMessageW(child_windows[CWI_ASSET_BRUSH_SIZE_THUMB], TBM_GETPOS, 0, 0);
		queue_add(internal.events->brush_size_handler, &brush_size);
		return true;
	}
	case INFO_BOX_MSG_STATE_READY:
	{
		TreeView_DeleteAllItems(child_windows[CWI_ASSET_TREEVIEW]);
		TreeView_DeleteAllItems(child_windows[CWI_ASSET_CURRENT_TREEVIEW]);
		asset_info_state_set_tree_view((asset_t*)wparam);
		break;
	}
	}
	return false;
}

bool asset_info_handle_interact_tree_item(bool is_global, element_t element)
{
	if (asset_can_change_field(serialize_element_get_value(element)))
	{
		asset_info_suite_t suite =
		{
			.internal = internal,
			.asset = asset,
			.action_buffer = action_buffer,
			.current_tool = current_tool,
			.palette_window = child_windows[CWI_ASSET_PALETTE],
			.tool_blocks = blocks,
			.selection_region = region
		};
		return asset_handle_interact_treeview(&suite, is_global, element);
	}
	return false;
}

void asset_info_set(asset_t* _asset)
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

void asset_info_set_current(region_t _region)
{
	region = region_is_invalid(_region) ? _region : region_validate(_region);
	asset_set_current_treeview_from_region(child_windows[CWI_ASSET_CURRENT_TREEVIEW], asset, region, &blocks[current_tool]);
}

action_buffer_t asset_info_action_buffer_get(void)
{
	return action_buffer;
}

void asset_info_action_buffer_set(action_buffer_t _action_buffer)
{
	action_buffer = _action_buffer;
}

info_tool_t asset_info_get_current_tool(void)
{
	return current_tool;
}

void asset_info_get_current_brush_block(asset_block_t* res)
{
	*res = blocks[TOOL_BRUSH];
}

int asset_info_get_current_brush_size(void)
{
	return brush_size;
}

void asset_info_directory_set(const char* _directory)
{
	FILE* file = fopen(_directory, "r");
	RUNTIME_ASSERT(file);
	fclose(file);

	snprintf(directory, sizeof directory, "%s", _directory);
	info_set_caption(path_get_file_name(directory));
}

void asset_info_directory_get(char* _directory, size_t buf_size)
{
	snprintf(_directory, buf_size, "%s", directory);
}

void asset_info_palette_save(asset_block_t* palette, size_t palette_size)
{
	int min = min((int)palette_size, MINERAL_PALETTE_GET_SIZE(child_windows[CWI_ASSET_PALETTE]));
	for (int i = 0; i < min; i++)
	{
		MINERAL_PALETTE_GET_CELL(child_windows[CWI_ASSET_PALETTE], i, &palette[i]);
	}
}

void asset_info_palette_copy(const asset_block_t* palette, size_t palette_size)
{
	int min = min((int)palette_size, MINERAL_PALETTE_GET_SIZE(child_windows[CWI_ASSET_PALETTE]));
	for (int i = 0; i < min; i++)
	{
		MINERAL_PALETTE_SET_CELL(child_windows[CWI_ASSET_PALETTE], i, &palette[i]);
	}
}