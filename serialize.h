/*
	serialize.h ~ RL
*/

#pragma once

#include "file.h"
#include <Windows.h>
#include <commctrl.h>

typedef struct element* element_t;

void serialize_single(const char* type, void* value, const char* name, HWND tree_window, HTREEITEM tree_item);
void serialize_array(const char* type, void* value, int count, const char* name, HWND tree_window, HTREEITEM tree_item);

void serialize_delete(HWND tree_window);

void serialize_on_expand(element_t element);
void serialize_on_change_field(element_t element);

HTREEITEM serialize_tree_find_item(HWND tree_window, HTREEITEM root, const char* name);