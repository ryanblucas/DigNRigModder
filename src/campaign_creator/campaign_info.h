/*
	campaign_info.h ~ RL
*/

#pragma once

#include "../info_box.h"
#include "campaign_main.h"

#define CAMPAIGN_IS_LAYER_FILE_CHANGE(directory) ((directory) >= 0 && (directory) < 14)
#define CAMPAIGN_CONVERT_FILE_CHANGE_PARAM(directory) (int)(directory)

void campaign_info_initialize(info_internal_t* internal);
void campaign_info_destroy(void);
void campaign_info_show(bool is_visible);
bool campaign_info_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, LRESULT* out);
bool campaign_info_handle_interact_tree_item(bool is_global, element_t element);

void campaign_info_set(campaign_t* campaign, const char* directory);
/* returns false if user cancels, true otherwise. this will set the caption of the window */
bool campaign_info_find_file(char* directory, size_t size);