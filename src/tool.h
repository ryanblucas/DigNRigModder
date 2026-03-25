/*
	tool.h ~ RL
*/

#pragma once

#include "types.h"

#define MAX_SELECTION_WIDTH 120
#define MAX_SELECTION_HEIGHT 80
#define MAX_SELECTION_SIZE (MAX_SELECTION_WIDTH * MAX_SELECTION_HEIGHT)

typedef struct tool_select* tool_select_t;

typedef enum tool_select_event
{
	EVENT_SELECTION_MOVE,
	EVENT_SELECTION_RESIZE,
	EVENT_SELECTION_RESIZE_STOP,
	EVENT_SELECTION_MOVE_START,
	EVENT_SELECTION_MOVE_STOP,
	EVENT_SELECTION_MOVE_CANCEL,
	EVENT_NOTHING,
} tool_select_event_t;

/* creates a selection tool */
tool_select_t tool_select_create(void);
/* frees memory associated to a selection tool. If tool is NULL, returns without error */
void tool_select_destroy(tool_select_t tool);
/* Resets state of tool */
void tool_select_reset(tool_select_t tool);
/* returns region of selection */
region_t tool_select_region(tool_select_t tool);
/* renders selection (invert what is selected or render what is being moved) */
region_t tool_select_render(tool_select_t tool);
/* Handles a mouse movement for a selection tool */
tool_select_event_t tool_select_handle_mouse_move(tool_select_t tool, bool m1_down, int x, int y, int scroll_y);
/* Handles a mouse click for a selection tool */
tool_select_event_t tool_select_handle_mouse_click(tool_select_t tool, bool m1_down, int x, int y, int scroll_y);