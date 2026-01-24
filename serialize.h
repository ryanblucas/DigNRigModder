/*
	serialize.h ~ RL
*/

#pragma once

#include "file.h"
#include <Windows.h>
#include <commctrl.h>

void serialize_single(const char* type, void* value, const char* name, HWND tree_window, HTREEITEM tree_item);
void serialize_array(const char* type, void* value, int count, const char* name, HWND tree_window, HTREEITEM tree_item);