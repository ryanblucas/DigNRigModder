/*
	tool.c ~ RL
*/

#include "tool.h"
#include "screen.h"

struct tool_select
{
	region_t selection;
	sprite_t selection_visual;
	int move_x, move_y, hinge_x, hinge_y;
	tool_event_t last_event;
	region_t arena;
};

struct tool_brush
{
	tool_brush_type_t type;
	size_t before_reserved;
	size_t before_count;
	tool_brush_element_t* before_buffer;
	int prev_selected_x, prev_selected_y;
	int size;
	region_t region;
	region_t arena;
	tool_brush_function_t callback;
	tool_event_t last_event;
};

tool_select_t tool_select_create(int width, int height)
{
	tool_select_t res = dig_malloc(sizeof * res);
	*res = (struct tool_select){ .selection = INVALID_REGION, .arena = { 0, 0, width - 1, height - 1 } };
	res->hinge_x = res->hinge_y = res->move_x = res->move_y = -1;
	return res;
}

void tool_select_destroy(tool_select_t tool)
{
	if (!tool)
	{
		return;
	}
	screen_sprite_destroy(tool->selection_visual);
	free(tool);
}

void tool_select_reset(tool_select_t tool)
{
	tool->hinge_x = tool->hinge_y = -1;
	tool->move_x = tool->move_y = -1;
	tool->selection = INVALID_REGION;
	screen_sprite_destroy(tool->selection_visual);
	tool->selection_visual = NULL;
}

region_t tool_select_region(const tool_select_t tool)
{
	return region_validate(tool->selection);
}

void tool_select_set_region(tool_select_t tool, region_t region)
{
	RUNTIME_ASSERT(!region_is_invalid(region));
	tool->selection = region_validate(region);
}

region_t tool_select_move_region(const tool_select_t tool)
{
	if (tool->hinge_x != -1 && tool->hinge_y != -1)
	{
		region_t src_region = region_validate(tool->selection);
		region_t dest_region =
		{
			tool->move_x - tool->hinge_x,
			tool->move_y - tool->hinge_y,
			tool->move_x - tool->hinge_x + region_width(src_region) - 1,
			tool->move_y - tool->hinge_y + region_height(src_region) - 1
		};
		return dest_region;
	}
	return INVALID_REGION;
}

void tool_select_render(const tool_select_t tool, int scroll_x, int scroll_y)
{
	if (region_is_invalid(tool->selection))
	{
		return;
	}
	region_t screen_region = region_validate(tool->selection);
	screen_region.x0 -= scroll_x;
	screen_region.x1 -= scroll_x;	
	screen_region.y0 -= scroll_y;
	screen_region.y1 -= scroll_y;
	if (tool->hinge_x >= 0 && tool->hinge_y >= 0)
	{
		attribute_t* selected = dig_malloc(region_size(screen_region) * sizeof * selected);
		memset(selected, 0, region_size(screen_region) * sizeof * selected);

		screen_set_attrib_region(selected, screen_region);
		screen_sprite_render(tool->move_x - tool->hinge_x - scroll_x, tool->move_y - tool->hinge_y - scroll_y, tool->selection_visual);
		free(selected);
		return;
	}
	screen_invert_region(screen_region);
}

static void tool_select_start_move(tool_select_t tool, int x, int y, int scroll_x, int scroll_y)
{
	region_t correct = region_validate(tool->selection);
	correct.x0 -= scroll_x;
	correct.x1 -= scroll_x;
	correct.y0 -= scroll_y;
	correct.y1 -= scroll_y;
	tool->hinge_x = x - correct.x0;
	tool->hinge_y = y - correct.y0;
	tool->move_x = x + scroll_x;
	tool->move_y = y + scroll_y;

	char* text = dig_malloc(region_size(tool->selection));
	attribute_t* attributes = dig_malloc(region_size(tool->selection) * sizeof * attributes);

	screen_get_char_region(text, correct);
	screen_get_attrib_region(attributes, correct);

	/* b/c it's selected rn */
	for (int i = 0; i < region_size(tool->selection); i++)
	{
		attributes[i] = ~attributes[i] & 0xFF;
	}

	tool->selection_visual = screen_sprite_create(region_width(tool->selection), region_height(tool->selection), 0, text, attributes);

	free(text);
	free(attributes);
}

static tool_event_t tool_select_stop_move(tool_select_t tool)
{
	region_t src_region = region_validate(tool->selection);
	region_t dest_region = tool_select_move_region(tool);
	if (memcmp(&src_region, &dest_region, sizeof src_region) != 0)
	{
		/* maintain state of tool until next time state is affected so that it can be referenced in response to this function */
		return EVENT_SELECTION_MOVE_STOP;
	}
	tool_select_reset(tool);
	return EVENT_SELECTION_MOVE_CANCEL;
}

tool_event_t tool_select_handle_mouse_move(tool_select_t tool, bool m1_down, int x, int y, int scroll_x, int scroll_y)
{
	if (tool->last_event == EVENT_SELECTION_MOVE_STOP)
	{
		tool_select_reset(tool);
	}

	if (!m1_down || region_is_invalid(tool->selection))
	{
		return tool->last_event = EVENT_NOTHING;
	}

	int new_selected_x = x + scroll_x;
	int new_selected_y = y + scroll_y;

	if (tool->hinge_x >= 0 && tool->hinge_y >= 0)
	{
		if (new_selected_x < 0 || new_selected_x >= region_width(tool->arena) || new_selected_y < 0 || new_selected_y >= region_height(tool->arena))
		{
			return tool->last_event = EVENT_SELECTION_MOVE;
		}
		tool->move_x = new_selected_x;
		tool->move_y = new_selected_y;
		return tool->last_event = EVENT_SELECTION_MOVE;
	}

	tool->selection.x1 = min(new_selected_x, tool->selection.x0 + MAX_SELECTION_WIDTH - 1);
	tool->selection.y1 = min(new_selected_y, tool->selection.y0 + MAX_SELECTION_HEIGHT - 1);
	tool->selection.x1 = max(tool->selection.x1, tool->selection.x0 - MAX_SELECTION_WIDTH + 1);
	tool->selection.y1 = max(tool->selection.y1, tool->selection.y0 - MAX_SELECTION_HEIGHT + 1);
	tool->selection = region_keep_inside(tool->arena, tool->selection);
	return tool->last_event = EVENT_SELECTION_RESIZE;
}

tool_event_t tool_select_handle_mouse_click(tool_select_t tool, bool m1_down, int x, int y, int scroll_x, int scroll_y)
{
	if (tool->last_event == EVENT_SELECTION_MOVE_STOP)
	{
		tool_select_reset(tool);
	}

	tool->last_event = EVENT_NOTHING;
	if (!m1_down)
	{
		if (tool->hinge_x >= 0 && tool->hinge_y >= 0)
		{
			tool->last_event = tool_select_stop_move(tool);
		}
		else if (!region_is_invalid(tool->selection))
		{
			tool->last_event = EVENT_SELECTION_RESIZE_STOP;
		}
		return tool->last_event;
	}

	if (!region_is_invalid(tool->selection) && region_is_inside(tool->selection, x + scroll_x, y + scroll_y))
	{
		tool_select_start_move(tool, x, y, scroll_x, scroll_y);
		return tool->last_event = EVENT_SELECTION_MOVE_START;
	}

	if (x + scroll_x < 0 || y + scroll_y < 0 || x + scroll_x >= region_width(tool->arena) || y + scroll_y >= region_height(tool->arena))
	{
		return tool->last_event = EVENT_NOTHING;
	}

	tool->selection.x0 = tool->selection.x1 = x + scroll_x;
	tool->selection.y0 = tool->selection.y1 = y + scroll_y;
	tool->selection = region_keep_inside(tool->arena, tool->selection);
	return tool->last_event = EVENT_SELECTION_RESIZE;
}

tool_brush_t tool_brush_create(tool_brush_function_t callback, tool_brush_type_t brush_type, int width, int height)
{
	tool_brush_t brush = dig_malloc(sizeof * brush);
	*brush = (struct tool_brush){ .region = INVALID_REGION, .callback = callback, .type = brush_type, .size = 1 };
	brush->before_reserved = MAX_SELECTION_SIZE;
	brush->before_buffer = dig_malloc(brush->before_reserved * sizeof * brush->before_buffer);
	brush->arena = (region_t){ 0, 0, width - 1, height - 1 };
	return brush;
}

void tool_brush_destroy(tool_brush_t tool)
{
	if (tool)
	{
		free(tool->before_buffer);
		free(tool);
	}
}

void tool_brush_reset(tool_brush_t tool)
{
	tool->before_count = 0;
	tool->region = INVALID_REGION;
	tool->prev_selected_x = tool->prev_selected_y = -1;
}

region_t tool_brush_region(const tool_brush_t tool)
{
	return tool->region;
}

int tool_brush_size(const tool_brush_t tool)
{
	return tool->size;
}

tool_brush_type_t tool_brush_type(const tool_brush_t tool)
{
	return tool->type;
}

static void tool_brush_try_reserve_more(tool_brush_t tool)
{
	if (tool->before_count < tool->before_reserved)
	{
		return;
	}
	size_t buf_size = tool->before_reserved * sizeof * tool->before_buffer;
	tool->before_reserved *= 2;
	tool_brush_element_t* next = dig_malloc(buf_size * 2);
	memcpy(next, tool->before_buffer, buf_size);
	free(tool->before_buffer);
	tool->before_buffer = next;
}

void tool_brush_add_to_before_list_cb(tool_brush_t tool, complete_block_t* element)
{
	RUNTIME_ASSERT(tool->type == BRUSH_TYPE_COMPLETE_BLOCK);
	for (int i = 0; i < tool->before_count; i++)
	{
		if (element->block.x == tool->before_buffer[i].block.block.x && element->block.y == tool->before_buffer[i].block.block.y)
		{
			return;
		}
	}

	tool->before_buffer[tool->before_count++] = (tool_brush_element_t){ *element };
	tool_brush_try_reserve_more(tool);
}

void tool_brush_copy_before_cb(const tool_brush_t tool, const dnr_state_t* save, complete_block_t* array)
{
	RUNTIME_ASSERT(tool->type == BRUSH_TYPE_COMPLETE_BLOCK);
	region_t region = region_validate(tool->region);
	game_copy(save, region, array);
	for (int i = 0; i < tool->before_count; i++)
	{
		tool_brush_element_t* curr = tool->before_buffer + i;
		array[(curr->block.block.x - tool->region.x0) + (curr->block.block.y - tool->region.y0) * region_width(tool->region)] = curr->block;
	}
}

void tool_brush_add_to_before_list_ab(tool_brush_t tool, asset_block_t* element, int x, int y)
{
	RUNTIME_ASSERT(tool->type == BRUSH_TYPE_ASSET_BLOCK);
	for (int i = 0; i < tool->before_count; i++)
	{
		if (x == tool->before_buffer[i].asset.x && y == tool->before_buffer[i].asset.y)
		{
			return;
		}
	}

	tool->before_buffer[tool->before_count++] = (tool_brush_element_t){ .asset.block = *element, .asset.x = x, .asset.y = y };
	tool_brush_try_reserve_more(tool);
}

void tool_brush_copy_before_ab(const tool_brush_t tool, const asset_t* asset, asset_block_t* array)
{
	RUNTIME_ASSERT(tool->type == BRUSH_TYPE_ASSET_BLOCK);
	region_t region = region_validate(tool->region);
	game_asset_copy(asset, region, array);
	for (int i = 0; i < tool->before_count; i++)
	{
		tool_brush_element_t* curr = tool->before_buffer + i;
		array[(curr->asset.x - tool->region.x0) + (curr->asset.y - tool->region.y0) * region_width(tool->region)] = curr->asset.block;
	}
}

void tool_brush_set_size(tool_brush_t tool, int size)
{
	tool->size = size;
}

tool_event_t tool_brush_handle_mouse_move(tool_brush_t tool, bool m1_down, int x, int y, int scroll_y)
{
	if (tool->last_event == EVENT_BRUSH_END)
	{
		tool_brush_reset(tool);
	}
	if (!m1_down || region_is_invalid(tool->region))
	{
		tool->prev_selected_x = tool->prev_selected_y = -1;
		return tool->last_event = EVENT_NOTHING;
	}

	int new_selected_x = x;
	int new_selected_y = y + scroll_y;
	int radius = tool->size - 1;

	region_t final = region_validate((region_t) { new_selected_x, new_selected_y, tool->prev_selected_x, tool->prev_selected_y });
	RUNTIME_ASSERT(dig_inside_bounds(final.x0, final.y0) && dig_inside_bounds(final.x1, final.y1));
	final.x0 -= radius;
	final.y0 -= radius;
	final.x1 += radius;
	final.y1 += radius;
	final = region_keep_inside(final, tool->arena);

	int cx = new_selected_x;
	int cy = new_selected_y;
	int dx = abs(cx - tool->prev_selected_x);
	int dy = -abs(cy - tool->prev_selected_y);
	int ix = cx < tool->prev_selected_x ? 1 : -1;
	int iy = cy < tool->prev_selected_y ? 1 : -1;

	int error = dx + dy;

	/* theres probably a way of doing this more efficiently for a line with a thickness */
	while (true)
	{
		tool->callback(tool, region_keep_inside((region_t) { cx - radius, cy - radius, cx + radius, cy + radius }, tool->arena));
		if (error * 2 <= dx)
		{
			if (cy == tool->prev_selected_y)
			{
				break;
			}
			cy += iy;
			error += dx;
		}
		if (error * 2 >= dy)
		{
			if (cx == tool->prev_selected_x)
			{
				break;
			}
			cx += ix;
			error += dy;
		}
	}

	tool->region = region_merge(tool->region, final);

	tool->prev_selected_x = new_selected_x;
	tool->prev_selected_y = new_selected_y;
	return tool->last_event = EVENT_BRUSH_MOVE;
}

tool_event_t tool_brush_handle_mouse_click(tool_brush_t tool, bool m1_down, int x, int y, int scroll_y)
{
	if (tool->last_event == EVENT_BRUSH_END)
	{
		tool_brush_reset(tool);
	}
	if (m1_down)
	{
		y += scroll_y;
		tool->prev_selected_x = x;
		tool->prev_selected_y = y;
		int radius = tool->size - 1;
		tool->region = region_keep_inside(
			(region_t) { x - radius, y - radius, x + radius, y + radius},
			tool->arena);
		return tool->last_event = EVENT_BRUSH_START;
	}

	if (region_is_invalid(tool->region))
	{
		return tool->last_event = EVENT_NOTHING;
	}

	return tool->last_event = EVENT_BRUSH_END;
}