/*
	layer_info.c ~ RL
*/

#include "layer_info.h"
#include "../path.h"

enum child_window_index
{
	CWI_LAYER_FILE_COMBOBOX,
	CWI_LAYER_FILE_COMBOBOX_LABEL,
	CWI_COUNT
};

static HWND child_windows[CWI_COUNT];
static info_internal_t internal;

void layer_info_initialize(const info_internal_t* _internal)
{
	internal = *_internal;

	child_windows[CWI_LAYER_FILE_COMBOBOX] = CreateWindowExW(0, WC_COMBOBOXW, NULL, WS_VSCROLL | CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE, 40, 25, 200, 200, internal.window, NULL, NULL, NULL);
	SendMessageW(child_windows[CWI_LAYER_FILE_COMBOBOX], WM_SETFONT, (WPARAM)internal.font_caption, 0);

	child_windows[CWI_LAYER_FILE_COMBOBOX_LABEL] = CreateWindowExW(0, L"STATIC", L"File:", WS_VISIBLE | WS_CHILD, 2, 25, 36, 18, internal.window, NULL, NULL, NULL);
	SendMessageW(child_windows[CWI_LAYER_FILE_COMBOBOX_LABEL], WM_SETFONT, (WPARAM)internal.font_caption, 0);

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
	}
	return false;
}