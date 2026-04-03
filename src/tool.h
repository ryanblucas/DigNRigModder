/*
	tool.h ~ RL
*/

#pragma once

#include "game.h"
#include "types.h"

#define MAX_SELECTION_WIDTH 120
#define MAX_SELECTION_HEIGHT 80
#define MAX_SELECTION_SIZE (MAX_SELECTION_WIDTH * MAX_SELECTION_HEIGHT)

typedef struct tool_select* tool_select_t;
typedef struct tool_brush* tool_brush_t;

typedef union tool_brush_element
{
	complete_block_t block;
	struct
	{
		asset_block_t block;
		int x;
		int y;
	} asset; 
} tool_brush_element_t;

typedef void (*tool_brush_function_t)(tool_brush_t tool, region_t region);

typedef enum tool_event
{
	EVENT_NOTHING,
	EVENT_SELECTION_MOVE,
	EVENT_SELECTION_RESIZE,
	EVENT_SELECTION_RESIZE_STOP,
	EVENT_SELECTION_MOVE_START,
	EVENT_SELECTION_MOVE_STOP,
	EVENT_SELECTION_MOVE_CANCEL,
	EVENT_BRUSH_MOVE,
	EVENT_BRUSH_START,
	EVENT_BRUSH_END
} tool_event_t;

typedef enum tool_brush_type
{
	BRUSH_TYPE_COMPLETE_BLOCK,
	BRUSH_TYPE_ASSET_BLOCK
} tool_brush_type_t;

/* creates a selection tool. Width and height are the dimensions of what the tool is working with */
tool_select_t tool_select_create(int width, int height);
/* frees memory associated to a selection tool. If tool is NULL, returns without error */
void tool_select_destroy(tool_select_t tool);
/* Resets state of tool */
void tool_select_reset(tool_select_t tool);

/* returns region of selection */
region_t tool_select_region(const tool_select_t tool);
/* sets region of selection */
void tool_select_set_region(tool_select_t tool, region_t region);
/* returns where the region will be moved to if it is being moved */
region_t tool_select_move_region(const tool_select_t tool);

/* renders selection (invert what is selected or render what is being moved) */
void tool_select_render(const tool_select_t tool, int scroll_y);
/* Handles a mouse movement for a selection tool */
tool_event_t tool_select_handle_mouse_move(tool_select_t tool, bool m1_down, int x, int y, int scroll_y);
/* Handles a mouse click for a selection tool */
tool_event_t tool_select_handle_mouse_click(tool_select_t tool, bool m1_down, int x, int y, int scroll_y);

/* creates a brush tool. Callback is called on each cell that is brushed over. 
   Each cell can be called on more than once. brush_type refers to what the
   brush will be painting. This is used to determine the size of the element in tool_brush_element_t.
   Width and height are the boundaries of the tool's operating region */
tool_brush_t tool_brush_create(tool_brush_function_t callback, tool_brush_type_t brush_type, int width, int height);
/* frees memory associated with a brush tool. if tool is NULL, returns without error */
void tool_brush_destroy(tool_brush_t tool);
/* Resets state of tool */
void tool_brush_reset(tool_brush_t tool);

/* returns region that brush has passed over */
region_t tool_brush_region(const tool_brush_t tool);
/* returns radius of brush */
int tool_brush_size(const tool_brush_t tool);
/* returns type of brush element */
tool_brush_type_t tool_brush_type(const tool_brush_t tool);

/* Adds element to brush list */
void tool_brush_add_to_before_list_cb(tool_brush_t tool, complete_block_t* element);
/* Sets array to a 2d array of complete blocks with dimensions of tool_brush_region. */
void tool_brush_copy_before_cb(const tool_brush_t tool, const dnr_state_t* save, complete_block_t* array);
/* Adds element to brush list */
void tool_brush_add_to_before_list_ab(tool_brush_t tool, asset_block_t* element, int x, int y);
/* Sets array to a 2d array of asset blocks with dimensions of tool_brush_region. */
void tool_brush_copy_before_ab(const tool_brush_t tool, const asset_t* asset, asset_block_t* array);

/* sets radius of brush */
void tool_brush_set_size(tool_brush_t tool, int size);

/* Handles a mouse movement for a brush tool */
tool_event_t tool_brush_handle_mouse_move(tool_brush_t tool, bool m1_down, int x, int y, int scroll_y);
/* Handles a mouse click for a brush tool */
tool_event_t tool_brush_handle_mouse_click(tool_brush_t tool, bool m1_down, int x, int y, int scroll_y);