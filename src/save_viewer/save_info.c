/*
	save_info.c ~ RL
	Info box side of save viewer
*/

#include "save_info.h"
#include "../action_buffer.h"
#include "../interface/charmap_control.h"
#include "../game.h"
#include "../interface/mineral_control.h"

#define INFO_BOX_CELL_SIZE 72
#define INFO_BOX_MSG_STATE_READY (WM_USER + 0x10)

static void save_info_state_set_tree_view(dnr_state_t* user_state);
static void save_info_state_update_current_cell_image(int x, int y);
static void save_info_state_brush_set_tree_view(void);

enum child_window_index
{
	CWI_SAVE_TREEVIEW,
	CWI_SAVE_CURRENT_CELL,
	CWI_SAVE_CURRENT_TREEVIEW,
	CWI_SAVE_BRUSH_CELL,
	CWI_SAVE_ERASE_BUTTON,
	CWI_SAVE_SELECT_BUTTON,
	CWI_SAVE_BRUSH_BUTTON,
	CWI_SAVE_BRUSH_SIZE_THUMB,

	CWI_COUNT
};

static action_buffer_t action_buffer;
static info_internal_t internal;
static HWND child_windows[CWI_COUNT];

static info_tool_t current_tool;

static dnr_state_t* state;
static int current_selection_index = -1;
static region_t current_selection_region;
static dnr_block_t current_block;
static dnr_mineral_t current_mineral;

static complete_block_t brush;
static int brush_size = 1;

void save_info_initialize(info_internal_t* _internal)
{
	current_selection_region = INVALID_REGION;
	internal = *_internal;

	mineral_control_initialize();
	charmap_control_initialize();

	child_windows[CWI_SAVE_TREEVIEW] = CreateWindowExW(0, WC_TREEVIEWW, NULL, WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS, 2, 190, INFO_BOX_CLIENT_WIDTH - 4, 200, internal.window, NULL, NULL, NULL);
	RUNTIME_ASSERT(child_windows[CWI_SAVE_TREEVIEW]);
	SendMessageW(child_windows[CWI_SAVE_TREEVIEW], WM_SETFONT, (WPARAM)internal.font_text, (LPARAM)FALSE);

	child_windows[CWI_SAVE_CURRENT_CELL] = CreateWindowExW(0, MINERAL_CONTROL_CLASS_NAME, NULL, WS_VISIBLE | WS_CHILD, 3, 25, 72, 72, internal.window, NULL, NULL, NULL);
	RUNTIME_ASSERT(child_windows[CWI_SAVE_CURRENT_CELL]);
	child_windows[CWI_SAVE_BRUSH_CELL] = CreateWindowExW(0, MINERAL_CONTROL_CLASS_NAME, NULL, WS_VISIBLE | WS_CHILD, 56, 170, 16, 16, internal.window, NULL, NULL, NULL);
	RUNTIME_ASSERT(child_windows[CWI_SAVE_BRUSH_CELL]);

	child_windows[CWI_SAVE_CURRENT_TREEVIEW] = CreateWindowExW(0, WC_TREEVIEWW, NULL, WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS, 76, 25, 372, 166, internal.window, NULL, NULL, NULL);
	RUNTIME_ASSERT(child_windows[CWI_SAVE_CURRENT_TREEVIEW]);
	SendMessageW(child_windows[CWI_SAVE_CURRENT_TREEVIEW], WM_SETFONT, (WPARAM)internal.font_text, (LPARAM)FALSE);

	child_windows[CWI_SAVE_ERASE_BUTTON] = CreateWindowExW(0, L"BUTTON", L"Erase", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 3, 98, 72, 22, internal.window, NULL, NULL, NULL);
	child_windows[CWI_SAVE_SELECT_BUTTON] = CreateWindowExW(0, L"BUTTON", L"Select", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 3, 121, 72, 22, internal.window, NULL, NULL, NULL);
	child_windows[CWI_SAVE_BRUSH_BUTTON] = CreateWindowExW(0, L"BUTTON", L"Brush", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 3, 144, 72, 22, internal.window, NULL, NULL, NULL);
	child_windows[CWI_SAVE_BRUSH_SIZE_THUMB] = CreateWindowExW(0, TRACKBAR_CLASSW, L"Brush size", WS_VISIBLE | WS_CHILD | TBS_AUTOTICKS | TBS_ENABLESELRANGE, 3, 167, 50, 22, internal.window, NULL, NULL, NULL);
	SendMessageW(child_windows[CWI_SAVE_BRUSH_SIZE_THUMB], TBM_SETRANGE, TRUE, MAKELONG(INFO_BRUSH_MIN_SIZE, INFO_BRUSH_MAX_SIZE));

	for (int i = 0; i < 4; i++)
	{
		RUNTIME_ASSERT(child_windows[CWI_SAVE_ERASE_BUTTON + i]);
		SendMessageW(child_windows[CWI_SAVE_ERASE_BUTTON + i], WM_SETFONT, (WPARAM)internal.font_text, (LPARAM)FALSE);
	}
	current_tool = TOOL_SELECT;
	EnableWindow(child_windows[CWI_SAVE_SELECT_BUTTON], FALSE);
	RAISE_EVENT(internal.events->tool_handler, current_tool);

	if (GetWindowLongPtr(internal.window, GWLP_USERDATA) == 1)
	{
		SendMessageW(internal.window, INFO_BOX_MSG_STATE_READY, (WPARAM)state, 0);
	}

	_internal->global_treeview = child_windows[CWI_SAVE_TREEVIEW];
	_internal->current_treeview = child_windows[CWI_SAVE_CURRENT_TREEVIEW];

	save_info_show(false);
}

void save_info_destroy(void)
{
	for (int i = 0; i < CWI_COUNT; i++)
	{
		DestroyWindow(child_windows[i]);
	}
	mineral_control_destroy();
	charmap_control_destroy();
}

void save_info_show(bool is_visible)
{
	for (int i = 0; i < CWI_COUNT; i++)
	{
		ShowWindow(child_windows[i], is_visible ? SW_SHOW : SW_HIDE);
	}
}

static void save_info_window_change_current(element_t element)
{
	if (current_tool == TOOL_BRUSH)
	{
		if (!serialize_on_change_field(element))
		{
			return;
		}
		complete_block_t copy = brush;
		CHAR_INFO cell = game_spritify_cell(&copy);
		RAISE_EVENT(internal.events->brush_block_handler, &copy);
		MINERAL_CONTROL_SET_CELL(child_windows[CWI_SAVE_BRUSH_CELL], cell.Char.AsciiChar, cell.Attributes, DNR_DEFAULT_DIRT_COLOR);
		return;
	}
	else if (current_selection_index >= 0)
	{
		int x = current_selection_index / WORLD_HEIGHT;
		int y = current_selection_index % WORLD_HEIGHT;
		region_t region = { x, y, x, y };

		complete_block_t start;
		game_copy(state, region, &start);
		action_buffer_pre_add_block(action_buffer, state, region);

		if (!serialize_on_change_field(element))
		{
			return;
		}

		complete_block_t end;
		game_copy(state, region, &end);
		if (memcmp(&start, &end, sizeof start) == 0)
		{
			return;
		}

		RAISE_EVENT(internal.events->block_handler, region);
		save_info_state_update_current_cell_image(x, y);
		action_buffer_post_add_block(action_buffer, state);
		return;
	}

	if (serialize_element_get_size(element) > 4)
	{
		return;
	}

	field_t previous = field_create(serialize_element_get_value(element), serialize_element_get_size(element));
	if (!serialize_on_change_field(element))
	{
		return;
	}
	if (previous == field_create(serialize_element_get_value(element), serialize_element_get_size(element)))
	{
		return;
	}
	action_buffer_pre_add_block(action_buffer, state, current_selection_region);
 	for (int y = current_selection_region.y0; y <= current_selection_region.y1; y++)
	{
		for (int x = current_selection_region.x0; x <= current_selection_region.x1; x++)
		{
			uint8_t* current = (uint8_t*)serialize_element_get_value(element);
			memcpy((uint8_t*)&state->blocks[x * WORLD_HEIGHT + y] + (current - (uint8_t*)&current_block), current, serialize_element_get_size(element));
		}
	}
	RAISE_EVENT(internal.events->block_handler, current_selection_region);
	action_buffer_post_add_block(action_buffer, state);
}

void save_info_handle_interact_tree_item(bool is_global, element_t element)
{
	if (!is_global)
	{
		save_info_window_change_current(element);
		return;
	}
	/* only should change elementary fields */
	if (serialize_element_get_size(element) > 4 || serialize_element_get_count(element) > 1)
	{
		return;
	}

	field_t begin_copy = field_create(serialize_element_get_value(element), serialize_element_get_size(element));
	if (!serialize_on_change_field(element))
	{
		return;
	}
	RAISE_EVENT(internal.events->global_field_handler, serialize_element_get_value(element));
	action_buffer_add_field(action_buffer, element, begin_copy);
}

bool save_info_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, LRESULT* out)
{
	*out = 0;
	switch (msg)
	{
	case WM_COMMAND:
	{
		if (HIWORD(wparam) != BN_CLICKED)
		{
			return true;
		}
		EnableWindow(child_windows[CWI_SAVE_ERASE_BUTTON + current_tool], TRUE);
		HWND button_clicked = (HWND)lparam;
		EnableWindow(button_clicked, FALSE);
		MINERAL_CONTROL_SET_CELL(child_windows[CWI_SAVE_CURRENT_CELL], 0, 0, 0);
		TreeView_DeleteAllItems(child_windows[CWI_SAVE_CURRENT_TREEVIEW]);
		if (button_clicked == child_windows[CWI_SAVE_ERASE_BUTTON])
		{
			current_tool = TOOL_ERASER;
		}
		else if (button_clicked == child_windows[CWI_SAVE_SELECT_BUTTON])
		{
			current_tool = TOOL_SELECT;
		}
		else if (button_clicked == child_windows[CWI_SAVE_BRUSH_BUTTON])
		{
			current_tool = TOOL_BRUSH;
			save_info_state_brush_set_tree_view();
		}
		RAISE_EVENT(internal.events->tool_handler, current_tool);
		return true;
	}
	case WM_HSCROLL:
	{
		if ((HWND)lparam != child_windows[CWI_SAVE_BRUSH_SIZE_THUMB] || LOWORD(wparam) != SB_ENDSCROLL)
		{
			return true;
		}
		brush_size = (int)SendMessageW(child_windows[CWI_SAVE_BRUSH_SIZE_THUMB], TBM_GETPOS, 0, 0);
		RAISE_EVENT(internal.events->brush_size_handler, brush_size);
		return true;
	}
	case INFO_BOX_MSG_STATE_READY:
		if (current_selection_index != -1)
		{
			save_info_cell_set_current(current_selection_index / WORLD_HEIGHT, current_selection_index % WORLD_HEIGHT);
		}
		serialize_delete(child_windows[CWI_SAVE_TREEVIEW]);
		save_info_state_set_tree_view((dnr_state_t*)wparam);
		return true;
	}
	return false;
}

action_buffer_t save_info_action_buffer_get(void)
{
	return action_buffer;
}

void save_info_action_buffer_set(action_buffer_t buffer)
{
	action_buffer = buffer;
}

static void save_info_state_set_tree_view(dnr_state_t* user_state)
{
	debug_profiler_push();
	dnr_state_t* item = user_state;

#define ADD_SERIALIZABLE(type, name) serialize_single(#type, &item->name, #name, child_windows[CWI_SAVE_TREEVIEW], NULL);
#define ADD_SERIALIZABLE_ARRAY(type, name, count) serialize_array(#type, &item->name, count, #name, child_windows[CWI_SAVE_TREEVIEW], NULL);

	SERIALIZABLE_DNR_STATE

#undef ADD_SERIALIZABLE
#undef ADD_SERIALIZABLE_ARRAY

	debug_profiler_pop("Surface level serializing");
}

dnr_state_t* save_info_state_get(void)
{
	return state;
}

void save_info_state_set(dnr_state_t* _state)
{
	state = _state;
	/* i do not like this */
	for (int i = 0; i < 100 && !internal.window; i++)
	{
		Sleep(10);
	}
	RUNTIME_ASSERT(internal.window);
	PostMessageW(internal.window, INFO_BOX_MSG_STATE_READY, (WPARAM)state, 0);
}

info_tool_t save_info_get_current_tool(void)
{
	return current_tool;
}

void save_info_get_current_brush_block(complete_block_t* res)
{
	*res = brush;
}

int save_info_get_current_brush_size(void)
{
	return brush_size;
}

static inline HTREEITEM save_info_node_create(HWND tree, const LPWSTR name)
{
	TVINSERTSTRUCTW tvins;
	tvins.hParent = TVI_ROOT;
	tvins.itemex.pszText = name;
	tvins.itemex.mask = TVIF_TEXT;
	tvins.hInsertAfter = TVI_LAST;
	return TreeView_InsertItem(tree, &tvins);
}

static void save_info_cell_set_current_treeview()
{
	int x = current_selection_index / WORLD_HEIGHT;
	int y = current_selection_index % WORLD_HEIGHT;

	dnr_block_t* block = &state->blocks[current_selection_index];
#define ADD_SERIALIZABLE(type, name) serialize_single(#type, &item->name, #name, child_windows[CWI_SAVE_CURRENT_TREEVIEW], root);
#define ADD_SERIALIZABLE_ARRAY(type, name, count) serialize_array(#type, &item->name, count, #name, child_windows[CWI_SAVE_CURRENT_TREEVIEW], root);

	{
		HTREEITEM root = save_info_node_create(child_windows[CWI_SAVE_CURRENT_TREEVIEW], L"block");
		dnr_block_t* item = block;
		SERIALIZABLE_DNR_BLOCK
		TreeView_Expand(child_windows[CWI_SAVE_CURRENT_TREEVIEW], root, TVE_EXPAND);
	}
	if (block->mineral_exists)
	{
		HTREEITEM root = save_info_node_create(child_windows[CWI_SAVE_CURRENT_TREEVIEW], L"mineral");
		RUNTIME_ASSERT(block->mineral_index >= 0 && block->mineral_index < sizeof state->minerals / sizeof * state->minerals);
		dnr_mineral_t* item = &state->minerals[block->mineral_index];
		SERIALIZABLE_DNR_MINERAL
		TreeView_Expand(child_windows[CWI_SAVE_CURRENT_TREEVIEW], root, TVE_EXPAND);
	}
	for (int i = 0; i < state->stalactite_count; i++)
	{
		if (state->stalactite_array[i].exists && (int)state->stalactite_array[i].x == x && (int)state->stalactite_array[i].y == y)
		{
			HTREEITEM root = save_info_node_create(child_windows[CWI_SAVE_CURRENT_TREEVIEW], L"stalactite");
			stalactite_t* item = &state->stalactite_array[i];
			SERIALIZABLE_STALACTITE
			TreeView_Expand(child_windows[CWI_SAVE_CURRENT_TREEVIEW], root, TVE_EXPAND);
			break;
		}
	}

#undef ADD_SERIALIZABLE
#undef ADD_SERIALIZABLE_ARRAY
}

static void save_info_state_update_current_cell_image(int x, int y)
{
	complete_block_t block;
	game_copy(state, (region_t) { x, y, x, y }, &block);
	CHAR_INFO cell = game_spritify_cell(&block);
	int layer_index = y / TARGET_HEIGHT;
	MINERAL_CONTROL_SET_CELL(child_windows[CWI_SAVE_CURRENT_CELL], cell.Char.AsciiChar, cell.Attributes, state->layer_headers[layer_index].dirt_color);
}

void save_info_cell_set_current(int x, int y)
{
	if (!state)
	{
		debug_format("Tried to set current cell with no state\n");
		return;
	}

	if (x == -1 && y == -1)
	{
		current_selection_index = -1;
		MINERAL_CONTROL_SET_CELL(child_windows[CWI_SAVE_CURRENT_CELL], 0, 0, 0);
		if (current_tool == TOOL_SELECT)
		{
			TreeView_DeleteAllItems(child_windows[CWI_SAVE_CURRENT_TREEVIEW]);
		}
		return;
	}

	RUNTIME_ASSERT(x >= 0 && y >= 0 && x < WORLD_WIDTH && y < WORLD_HEIGHT);

	current_selection_region = INVALID_REGION;
	current_selection_index = x * WORLD_HEIGHT + y;

	if (current_tool != TOOL_SELECT)
	{
		return;
	}

	int pos = GetScrollPos(child_windows[CWI_SAVE_CURRENT_TREEVIEW], SB_VERT);
	TreeView_DeleteAllItems(child_windows[CWI_SAVE_CURRENT_TREEVIEW]);

	save_info_state_update_current_cell_image(x, y);
	save_info_cell_set_current_treeview();

	SetScrollPos(child_windows[CWI_SAVE_CURRENT_TREEVIEW], SB_VERT, pos, TRUE);
}

static void save_info_cell_set_current_region_treeview(dnr_block_t* block, dnr_mineral_t* mineral)
{
#define ADD_SERIALIZABLE(type, name) serialize_single(#type, &item->name, #name, child_windows[CWI_SAVE_CURRENT_TREEVIEW], root);

	{
		HTREEITEM root = save_info_node_create(child_windows[CWI_SAVE_CURRENT_TREEVIEW], L"block");
		dnr_block_t* item = block;
		ADD_SERIALIZABLE(int32_t, health_percentage);
		ADD_SERIALIZABLE(int32_t, health_max);
		ADD_SERIALIZABLE(int32_t, health_current);
		ADD_SERIALIZABLE(CHAR_INFO, visual);
		ADD_SERIALIZABLE(boolean32_t, block_exists);
		ADD_SERIALIZABLE(boolean32_t, can_mine);
		ADD_SERIALIZABLE(boolean32_t, mineral_exists);
		ADD_SERIALIZABLE(dnr_rig_type_t, rig_type);
		ADD_SERIALIZABLE(dnr_mineral_move_direction_t, mineral_move_direction);
		ADD_SERIALIZABLE(int32_t, damage);
		ADD_SERIALIZABLE(dnr_mineral_spawn_rule_t, mineral_spawn_rule);
		ADD_SERIALIZABLE(boolean32_t, can_remove_rig);
		TreeView_Expand(child_windows[CWI_SAVE_CURRENT_TREEVIEW], root, TVE_EXPAND);
	}

	if (mineral)
	{
		HTREEITEM root = save_info_node_create(child_windows[CWI_SAVE_CURRENT_TREEVIEW], L"mineral");
		dnr_mineral_t* item = mineral;
		ADD_SERIALIZABLE(dnr_mineral_size_t, size);
		ADD_SERIALIZABLE(dnr_mineral_type_t, type);
		TreeView_Expand(child_windows[CWI_SAVE_CURRENT_TREEVIEW], root, TVE_EXPAND);
	}

#undef ADD_SERIALIZABLE
}

#define CHECK_CONDITION(name) \
if (mask & (1 << position) && item1->name != item2->name) \
{ \
	mask = mask ^ (1 << position); \
	HTREEITEM prev = serialize_tree_find_item(window, root, #name); \
	if (prev) \
	{ \
		serialize_element_enable(serialize_element_get_from_node(window, prev), false); \
	} \
} \
position++;

static int save_info_copy_similar_block_data(HWND window, HTREEITEM root, int mask, dnr_block_t* item1, const dnr_block_t* item2)
{
	int position = 0;

	CHECK_CONDITION(health_percentage);
	CHECK_CONDITION(health_max);
	CHECK_CONDITION(health_current);
	if (mask & (1 << position) && (item1->visual.Char.AsciiChar != item2->visual.Char.AsciiChar || item1->visual.Attributes != item2->visual.Attributes))
	{
		mask = mask ^ (1 << position);
		HTREEITEM prev = serialize_tree_find_item(window, root, "visual");
		if (prev)
		{
			serialize_element_enable(serialize_element_get_from_node(window, prev), false);
		}
	}
	position++;
	CHECK_CONDITION(block_exists);
	CHECK_CONDITION(can_mine);
	CHECK_CONDITION(mineral_exists);
	CHECK_CONDITION(rig_type);
	CHECK_CONDITION(mineral_move_direction);
	CHECK_CONDITION(damage);
	CHECK_CONDITION(mineral_spawn_rule);
	CHECK_CONDITION(can_remove_rig);

	return mask;
}

static int save_info_copy_similar_mineral_data(HWND window, HTREEITEM root, int mask, dnr_mineral_t* item1, const dnr_mineral_t* item2)
{
	if (!item1 || !item1->exists)
	{
		return mask;
	}
	dnr_mineral_t temp = { 0 };
	if (!item2)
	{
		item1->exists = false;
		/* always going to fail for check below */
		temp.size = item1->size + 1;
		temp.type = item1->type + 1;
		item2 = &temp;
	}
	int position = 0;

	CHECK_CONDITION(size);
	CHECK_CONDITION(type);

	return mask;
}

#undef CHECK_CONDITION

void save_info_cell_set_current_region(region_t region)
{
	if (!state)
	{
		debug_format("Tried to set current cell region with no state\n");
		return;
	}

	if (region_is_invalid(region))
	{
		if (current_tool == TOOL_SELECT)
		{
			TreeView_DeleteAllItems(child_windows[CWI_SAVE_CURRENT_TREEVIEW]);
		}
		return;
	}

	region = region_validate(region);
	RUNTIME_ASSERT(dig_inside_bounds(region.x0, region.y0) && dig_inside_bounds(region.x1, region.y1));

	current_selection_index = -1;
	current_selection_region = region;

	current_block = state->blocks[region.x0 * WORLD_HEIGHT + region.y0];
	current_mineral = current_block.mineral_exists ? state->minerals[current_block.mineral_index] : (dnr_mineral_t) { 0 };

	if (current_tool != TOOL_SELECT)
	{
		return;
	}

	debug_profiler_push();

	TreeView_DeleteAllItems(child_windows[CWI_SAVE_CURRENT_TREEVIEW]);
	save_info_cell_set_current_region_treeview(&current_block, &current_mineral);

	int block_mask = ~0, mineral_mask = ~0;
	HWND window = child_windows[CWI_SAVE_CURRENT_TREEVIEW];
	HTREEITEM block_root = serialize_tree_find_item(window, NULL, "block");
	HTREEITEM mineral_root = serialize_tree_find_item(window, NULL, "mineral");

	/* no stalactite because that's just a waste of time... */
	for (int y = region.y0; y <= region.y1; y++)
	{
		for (int x = region.x0; x <= region.x1; x++)
		{
			dnr_block_t* block = &state->blocks[x * WORLD_HEIGHT + y];
			block_mask = save_info_copy_similar_block_data(window, block_root, block_mask, &current_block, block);
			mineral_mask = save_info_copy_similar_mineral_data(window, mineral_root, mineral_mask, &current_mineral, block->mineral_exists ? &state->minerals[block->mineral_index] : NULL);
		}
	}

	if (!current_mineral.exists)
	{
		HWND window = child_windows[CWI_SAVE_CURRENT_TREEVIEW];
		HTREEITEM root = serialize_tree_find_item(window, NULL, "mineral");
		serialize_element_delete(serialize_element_get_from_node(window, serialize_tree_find_item(window, root, "size")));
		serialize_element_delete(serialize_element_get_from_node(window, serialize_tree_find_item(window, root, "type")));
		TreeView_DeleteItem(window, root);
	}

	debug_profiler_pop("Setting region");
}

element_t save_info_element_find(bool global, const char* query)
{
	if (global)
	{
		return serialize_element_get_from_node(child_windows[CWI_SAVE_TREEVIEW],
			serialize_tree_find_item(child_windows[CWI_SAVE_TREEVIEW], TVI_ROOT, query));
	}
	return serialize_element_get_from_node(child_windows[CWI_SAVE_CURRENT_TREEVIEW],
		serialize_tree_find_item(child_windows[CWI_SAVE_CURRENT_TREEVIEW], TVI_ROOT, query));
}

static void save_info_state_brush_set_tree_view(void)
{
#define ADD_SERIALIZABLE(type, name) serialize_single(#type, &item->name, #name, child_windows[CWI_SAVE_CURRENT_TREEVIEW], root);
#define ADD_SERIALIZABLE_ARRAY(type, name, count) serialize_array(#type, &item->name, count, #name, child_windows[CWI_SAVE_CURRENT_TREEVIEW], root);
	{
		HTREEITEM root = save_info_node_create(child_windows[CWI_SAVE_CURRENT_TREEVIEW], L"block");
		dnr_block_t* item = &brush.block;
		SERIALIZABLE_DNR_BLOCK
		TreeView_Expand(child_windows[CWI_SAVE_CURRENT_TREEVIEW], root, TVE_EXPAND);
	}
	{
		HTREEITEM root = save_info_node_create(child_windows[CWI_SAVE_CURRENT_TREEVIEW], L"mineral");
		dnr_mineral_t* item = &brush.mineral;
		SERIALIZABLE_DNR_MINERAL
		TreeView_Expand(child_windows[CWI_SAVE_CURRENT_TREEVIEW], root, TVE_EXPAND);
	}
	{
		HTREEITEM root = save_info_node_create(child_windows[CWI_SAVE_CURRENT_TREEVIEW], L"stalactite");
		stalactite_t* item = &brush.stalactite;
		SERIALIZABLE_STALACTITE
		TreeView_Expand(child_windows[CWI_SAVE_CURRENT_TREEVIEW], root, TVE_EXPAND);
	}
#undef ADD_SERIALIZABLE
#undef ADD_SERIALIZABLE_ARRAY
}