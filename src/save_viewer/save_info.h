/*
	save_info.h ~ RL
	Info box side of save viewer
*/

#pragma once

#include "../action_buffer.h"
#include "../file.h"
#include "../info_box.h"
#include "../serialize.h"
#include "../types.h"
#include <Windows.h>

void save_info_initialize(const info_internal_t* internal);
void save_info_destroy(void);

void save_info_show(bool is_visible);
bool save_info_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, LRESULT* out);

action_buffer_t save_info_action_buffer_get(void);
void save_info_action_buffer_set(action_buffer_t buffer);

dnr_state_t* save_info_state_get(void);
void save_info_state_set(dnr_state_t* state);

info_tool_t save_info_get_current_tool(void);
void save_info_get_current_brush_block(complete_block_t* res);
int save_info_get_current_brush_size(void);

void save_info_cell_set_current(int x, int y);
void save_info_cell_set_current_region(region_t region);

element_t save_info_element_find(bool global, const char* query);