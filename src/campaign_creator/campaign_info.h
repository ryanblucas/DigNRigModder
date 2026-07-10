/*
	campaign_info.h ~ RL
*/

#pragma once

#include "../action_buffer.h"
#include "../info_box.h"
#include "campaign_main.h"
#include <stdint.h>

#define CAMPAIGN_IS_LAYER_FILE_CHANGE(directory) ((uintptr_t)(directory) >= 0 && (uintptr_t)(directory) < 14)
#define CAMPAIGN_CONVERT_FILE_CHANGE_PARAM(directory) ((int)((uintptr_t)(directory)))
#define CAMPAIGN_CONVERT_FILE_CHANGE_PARAM_INFO(index) ((const void*)((uintptr_t)(index)))

void campaign_info_initialize(info_internal_t* internal);
void campaign_info_destroy(void);
void campaign_info_show(bool is_visible);
bool campaign_info_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, LRESULT* out);
bool campaign_info_handle_interact_tree_item(bool is_global, element_t element);

action_buffer_t campaign_info_action_buffer_get(void);
void campaign_info_action_buffer_set(action_buffer_t action_buffer);

void campaign_info_set(campaign_t* campaign, const char* directory);
/* returns false if user cancels, true otherwise. this will set the caption of the window */
bool campaign_info_find_file(char* directory, size_t size);

info_tool_t campaign_info_get_tool(void);
campaign_mode_t campaign_mode(void);

void campaign_set_current_layer(asset_t* asset);
void campaign_set_current_region(region_t region);