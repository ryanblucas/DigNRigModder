/*
	asset_util.h ~ RL
*/

#pragma once

#include "../action_buffer.h"
#include "../file.h"
#include "../screen.h"
#include "../tool.h"

typedef struct asset_suite
{
	asset_t asset;
	tool_select_t tool_select;
	tool_brush_t tool_eraser;
	tool_brush_t tool_brush;
	action_buffer_t buffer;
	int scroll_x, scroll_y;
	region_t clipboard_region;
	asset_block_t* clipboard_data;
} asset_suite_t;

void asset_render(const asset_t* asset, bool is_layer, int x, int y);

tool_event_t asset_select_handle_mouse_button(asset_suite_t* suite, bool m1_down, int x, int y);
tool_event_t asset_brush_handle_mouse_button(asset_suite_t* suite, bool is_eraser, bool m1_down, int x, int y);
void asset_handle_brush(asset_suite_t* suite, region_t region, asset_block_t block);
void asset_handle_erase(asset_suite_t* suite, region_t region);
void asset_handle_copy(asset_suite_t* suite);
void asset_handle_paste(asset_suite_t* suite);