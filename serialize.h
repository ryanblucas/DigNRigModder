/*
	serialize.h ~ RL
*/

#pragma once

#include "file.h"
#include <Windows.h>
#include <commctrl.h>

typedef struct surface_array
{
	bool exists;
	uint64_t type_hash;
	void* value;
	int count;
	WCHAR name[64];
} surface_array_t;

bool serialize_is_surface_mode(void);
void serialize_set_preview_mode(bool mode);
void serialize_finish_array(surface_array_t* sa, HWND tree_window, HTREEITEM tree_item);

void serialize_single(const char* type, void* value, const char* name, HWND tree_window, HTREEITEM tree_item);
void serialize_array(const char* type, void* value, int count, const char* name, HWND tree_window, HTREEITEM tree_item);

void serialize_delete(HWND tree_window);