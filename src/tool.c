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
	tool_select_event_t last_event;
};

tool_select_t tool_select_create(void)
{
	tool_select_t res = dig_malloc(sizeof * res);
	*res = (struct tool_select){ .selection = INVALID_REGION };
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

region_t tool_select_region(tool_select_t tool)
{
	return region_validate(tool->selection);
}

region_t tool_select_render(tool_select_t tool)
{
	if (tool->hinge_x >= 0 && tool->hinge_y >= 0)
	{
		region_t screen_region = region_validate(tool->selection);

		attribute_t* selected = dig_malloc(region_size(screen_region) * sizeof * selected);
		memset(selected, 0, region_size(screen_region) * sizeof * selected);

		screen_set_attrib_region(selected, screen_region);
		screen_sprite_render(tool->move_x - tool->hinge_x, tool->move_y - tool->hinge_y, tool->selection_visual);
		free(selected);
		return;
	}
	screen_invert_region(region_validate(tool->selection));
}

static void tool_select_start_move(tool_select_t tool, int x, int y, int scroll_y)
{
	region_t correct = region_validate(tool->selection);
	correct.y0 -= scroll_y;
	correct.y1 -= scroll_y;
	tool->hinge_x = x - correct.x0;
	tool->hinge_y = y - correct.y0;
	tool->move_x = x;
	tool->move_y = y;

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

static tool_select_event_t tool_select_stop_move(tool_select_t tool)
{
	region_t src_region = region_validate(tool->selection);
	region_t dest_region =
	{
		tool->move_x - tool->hinge_x,
		tool->move_y - tool->hinge_y,
		tool->move_x - tool->hinge_x + region_width(src_region) - 1,
		tool->move_y - tool->hinge_y + region_height(src_region) - 1
	};

	if (memcmp(&src_region, &dest_region, sizeof src_region) != 0)
	{
		return EVENT_SELECTION_MOVE_STOP;
	}
	tool_select_reset(tool);
	return EVENT_SELECTION_MOVE_CANCEL;
}

tool_select_event_t tool_select_handle_mouse_move(tool_select_t tool, bool m1_down, int x, int y, int scroll_y)
{
	if (tool->last_event == EVENT_SELECTION_MOVE_STOP)
	{
		tool_select_reset(tool);
	}

	if (!m1_down || region_is_invalid(tool->selection))
	{
		return tool->last_event = EVENT_NOTHING;
	}

	int new_selected_x = x;
	int new_selected_y = y + scroll_y;

	if (tool->hinge_x >= 0 && tool->hinge_y >= 0)
	{
		tool->move_x = new_selected_x;
		tool->move_y = new_selected_y;
		return tool->last_event = EVENT_SELECTION_MOVE;
	}

	tool->selection.x1 = min(new_selected_x, tool->selection.x0 + MAX_SELECTION_WIDTH - 1);
	tool->selection.y1 = min(new_selected_y, tool->selection.y0 + MAX_SELECTION_HEIGHT - 1);
	tool->selection.x1 = max(tool->selection.x1, tool->selection.x0 - MAX_SELECTION_WIDTH + 1);
	tool->selection.y1 = max(tool->selection.y1, tool->selection.y0 - MAX_SELECTION_HEIGHT + 1);
	return tool->last_event = EVENT_SELECTION_RESIZE;
}

tool_select_event_t tool_select_handle_mouse_click(tool_select_t tool, bool m1_down, int x, int y, int scroll_y)
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
		else if (!region_is_invalid(tool->selection) && region_size(tool->selection) > 1)
		{
			tool->last_event = EVENT_SELECTION_RESIZE_STOP;
		}
		return tool->last_event;
	}

	if (!region_is_invalid(tool->selection) && region_is_inside(tool->selection, x, y))
	{
		tool_select_start_move(tool, x, y, scroll_y);
		return tool->last_event = EVENT_SELECTION_MOVE_START;
	}

	tool->selection.x0 = tool->selection.x1 = x;
	tool->selection.y0 = tool->selection.y1 = y + scroll_y;
	return tool->last_event = EVENT_SELECTION_RESIZE;
}