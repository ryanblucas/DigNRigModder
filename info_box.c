/*
	info_box.c ~ RL
	Window that increases interactivity with the main display
*/

#include "info_box.h"
#include "action_buffer.h"
#include "charmap_control.h"
#include "debug.h"
#include "mineral_control.h"
#include "serialize.h"
#include "types.h"

#include <Windows.h>
#include <commctrl.h>
#include <strsafe.h>

#define RAISE_EVENT(ev, ...) if (ev) ev(__VA_ARGS__);

#define INFO_BOX_CLASS_NAME L"dnr_mod_info"
#define INFO_BOX_WINDOW_STYLE (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX)
#define INFO_BOX_WINDOW_STYLE_EX (WS_EX_OVERLAPPEDWINDOW)
#define INFO_BOX_CLIENT_WIDTH 450
#define INFO_BOX_CLIENT_HEIGHT 400
#define INFO_BOX_CELL_SIZE 72

#define INFO_BOX_MSG_STATE_READY (WM_USER + 0x10)

static void info_state_set_tree_view(dnr_state_t* user_state);
static void info_state_update_current_cell_image(int x, int y);

enum child_window_index
{
	CWI_SAVE_TREEVIEW,
	CWI_SAVE_CURRENT_CELL,
	CWI_SAVE_CURRENT_TREEVIEW,
	CWI_SAVE_GO_TO_LAYER_BUTTON,

	CWI_SAVE_START = CWI_SAVE_TREEVIEW,
	CWI_SAVE_END = CWI_SAVE_GO_TO_LAYER_BUTTON,

	CWI_COUNT
};

static HFONT font_caption; 
static HFONT font_text;

static HWND window;
static HWND tab_control;

static HANDLE thread;
static DWORD thread_id;

static info_mode_t current_mode;
static HWND child_windows[CWI_COUNT];

static dnr_state_t* state;
static int current_selection_index = -1;
static region_t current_selection_region;
static dnr_block_t current_block;
static dnr_mineral_t current_mineral;

static info_events_t events;

static inline void info_tab_create(const LPWSTR name, info_mode_t index)
{
	TCITEMW tab = { .mask = TCIF_TEXT, .pszText = name };
	TabCtrl_InsertItem(tab_control, index, &tab);
}

static void info_tab_save(HWND hwnd)
{
	if (!child_windows[CWI_SAVE_TREEVIEW])
	{
		RECT rect = { 0, 0, INFO_BOX_CLIENT_WIDTH, INFO_BOX_CLIENT_HEIGHT };
		TabCtrl_AdjustRect(tab_control, FALSE, &rect);

		child_windows[CWI_SAVE_TREEVIEW] = CreateWindowExW(0, WC_TREEVIEWW, NULL, WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS, 2, 198, INFO_BOX_CLIENT_WIDTH - 4, 200, hwnd, NULL, NULL, NULL);
		RUNTIME_ASSERT(child_windows[CWI_SAVE_TREEVIEW]);
		SendMessageW(child_windows[CWI_SAVE_TREEVIEW], WM_SETFONT, (WPARAM)font_text, (LPARAM)FALSE);

		child_windows[CWI_SAVE_CURRENT_CELL] = CreateWindowExW(0, MINERAL_CONTROL_CLASS_NAME, NULL, WS_VISIBLE | WS_CHILD, 3, 25, 72, 72, hwnd, NULL, NULL, NULL);
		RUNTIME_ASSERT(child_windows[CWI_SAVE_CURRENT_CELL]);

		child_windows[CWI_SAVE_CURRENT_TREEVIEW] = CreateWindowExW(0, WC_TREEVIEWW, NULL, WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS, 76, 25, 372, 174, hwnd, NULL, NULL, NULL);
		RUNTIME_ASSERT(child_windows[CWI_SAVE_CURRENT_TREEVIEW]);
		SendMessageW(child_windows[CWI_SAVE_CURRENT_TREEVIEW], WM_SETFONT, (WPARAM)font_text, (LPARAM)FALSE);

		child_windows[CWI_SAVE_GO_TO_LAYER_BUTTON] = CreateWindowExW(0, L"BUTTON", L"Go to layer", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 3, 98, 72, 22, hwnd, NULL, NULL, NULL);
		RUNTIME_ASSERT(child_windows[CWI_SAVE_GO_TO_LAYER_BUTTON]);

		SendMessageW(child_windows[CWI_SAVE_GO_TO_LAYER_BUTTON], WM_SETFONT, (WPARAM)font_text, (LPARAM)FALSE);
		EnableWindow(child_windows[CWI_SAVE_GO_TO_LAYER_BUTTON], FALSE);
	}

	for (int i = CWI_SAVE_START; i <= CWI_SAVE_END; i++)
	{
		ShowWindow(child_windows[i], SW_SHOW);
	}

	if (GetWindowLongPtr(window, GWLP_USERDATA) == 1)
	{
		SendMessageW(window, INFO_BOX_MSG_STATE_READY, (WPARAM)state, 0);
	}
}

static void info_tab_sprite(HWND hwnd)
{

}

static void info_tab_layer(HWND hwnd)
{

}

static LRESULT info_window_tab_control_proc(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
	NMHDR* nmhdr = (NMHDR*)lparam;
	if (nmhdr->code != TCN_SELCHANGE)
	{
		return 0;
	}
	current_mode = TabCtrl_GetCurSel(tab_control);
	for (int i = 0; i < CWI_COUNT; i++)
	{
		ShowWindow(child_windows[i], SW_HIDE);
	}
	switch (current_mode)
	{
	case MODE_SAVE:
		info_tab_save(hwnd);
		break;
	case MODE_SPRITE:
		info_tab_sprite(hwnd);
		break;
	case MODE_LAYER:
		info_tab_layer(hwnd);
		break;
	default:
		RUNTIME_ASSERT(false);
		break;
	}
	RAISE_EVENT(events.mode_handler, current_mode);
	return 0;
}

static void info_window_save_tree_control_handle_double_click(HWND hwnd, HTREEITEM res)
{
	TVITEMEXW tvi;
	tvi.hItem = res;
	tvi.mask = TVIF_PARAM;
	TreeView_GetItem(hwnd, &tvi);
	element_t element = (element_t)tvi.lParam;

	if (hwnd == child_windows[CWI_SAVE_CURRENT_TREEVIEW] && current_selection_index >= 0)
	{
		int x = current_selection_index / WORLD_HEIGHT;
		int y = current_selection_index % WORLD_HEIGHT;
		region_t region = { x, y, x, y };
		action_buffer_pre_add_block(state, region);

		if (!serialize_on_change_field(element))
		{
			return 0;
		}

		RAISE_EVENT(events.block_handler, region);
		info_state_update_current_cell_image(x, y);
		action_buffer_post_add_block(state);
	}
	else if (hwnd == child_windows[CWI_SAVE_CURRENT_TREEVIEW])
	{
		if (!serialize_on_change_field(element))
		{
			return 0;
		}

		action_buffer_pre_add_block(state, current_selection_region);
		for (int y = current_selection_region.y0; y <= current_selection_region.y1; y++)
		{
			for (int x = current_selection_region.x0; x <= current_selection_region.x1; x++)
			{
				uint8_t* current = (uint8_t*)serialize_element_get_value(element);
				memcpy((uint8_t*)&state->blocks[x * WORLD_HEIGHT + y] + (current - (uint8_t*)&current_block), current, serialize_element_get_size(element));
			}
		}
		RAISE_EVENT(events.block_handler, current_selection_region);
		action_buffer_post_add_block(state);
	}
	else if (hwnd == child_windows[CWI_SAVE_TREEVIEW])
	{
		field_t begin_copy = field_create(serialize_element_get_value(element), serialize_element_get_size(element));

		if (!serialize_on_change_field(element))
		{
			return 0;
		}

		RAISE_EVENT(events.global_field_handler, serialize_element_get_value(element));
		action_buffer_add_field(element, begin_copy);
	}
}

static LRESULT info_window_save_tree_control_proc(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
	NMTREEVIEWW* nmtv = (NMTREEVIEWW*)lparam;
	if (nmtv->hdr.code == TVN_ITEMEXPANDING && nmtv->action == TVE_EXPAND && nmtv->itemNew.mask & TVIF_PARAM)
	{
		serialize_on_expand((element_t)nmtv->itemNew.lParam);
	}
	else if (nmtv->hdr.code == NM_DBLCLK)
	{
		/* the LPARAM here is NOT a NMTREEVIEWW */
		TVHITTESTINFO info;
		GetCursorPos(&info.pt);
		ScreenToClient(nmtv->hdr.hwndFrom, &info.pt);

		HTREEITEM res = TreeView_HitTest(nmtv->hdr.hwndFrom, &info);
		if (res && info.flags & TVHT_ONITEM)
		{
			info_window_save_tree_control_handle_double_click(nmtv->hdr.hwndFrom, res);
		}
	}
	return 0;
}

static LRESULT info_window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_CREATE:
	{
		tab_control = CreateWindowExW(0, WC_TABCONTROLW, NULL, WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS, 0, 0, INFO_BOX_CLIENT_WIDTH, INFO_BOX_CLIENT_HEIGHT, hwnd, NULL, NULL, NULL);
		RUNTIME_ASSERT(tab_control);

		info_tab_create(L"Save", MODE_SAVE);
		info_tab_create(L"Sprite", MODE_SPRITE);
		info_tab_create(L"Layer", MODE_LAYER);

		info_tab_save(hwnd);
		current_mode = MODE_SAVE;

		SendMessageW(tab_control, WM_SETFONT, (WPARAM)font_caption, FALSE);
		return 0;
	}
	case WM_NOTIFY:
	{
		NMHDR* nmhdr = (NMHDR*)lparam;
		if (nmhdr->hwndFrom == tab_control)
		{
			return info_window_tab_control_proc(hwnd, wparam, lparam);
		}
		else if (nmhdr->hwndFrom == child_windows[CWI_SAVE_TREEVIEW] || nmhdr->hwndFrom == child_windows[CWI_SAVE_CURRENT_TREEVIEW])
		{
			return info_window_save_tree_control_proc(hwnd, wparam, lparam);
		}
		return 0;
	}
	case WM_COMMAND:
	{
		if (HIWORD(wparam) != BN_CLICKED || current_selection_index == -1)
		{
			return 0;
		}
		HWND button_clicked = (HWND)lparam;
		if (button_clicked == child_windows[CWI_SAVE_GO_TO_LAYER_BUTTON])
		{
			char buf[64];
			snprintf(buf, sizeof buf, "layer_headers - %i", LAYER_COUNT);
			HTREEITEM headers = serialize_tree_find_item(child_windows[CWI_SAVE_TREEVIEW], TVI_ROOT, buf);
			TreeView_Expand(child_windows[CWI_SAVE_TREEVIEW], headers, TVE_EXPAND);
			snprintf(buf, sizeof buf, "%i", current_selection_index % WORLD_HEIGHT / TARGET_HEIGHT);
			TreeView_Expand(child_windows[CWI_SAVE_TREEVIEW], serialize_tree_find_item(child_windows[CWI_SAVE_TREEVIEW], headers, buf), TVE_EXPAND);
		}
		return 0;
	}
	case INFO_BOX_MSG_STATE_READY:
		if (!child_windows[CWI_SAVE_TREEVIEW])
		{
			SetWindowLongPtr(hwnd, GWLP_USERDATA, 1);
			return 0;
		}
		if (current_selection_index != -1)
		{
			info_cell_set_current(current_selection_index / WORLD_HEIGHT, current_selection_index % WORLD_HEIGHT);
		}
		serialize_delete(child_windows[CWI_SAVE_TREEVIEW]);
		info_state_set_tree_view((dnr_state_t*)wparam);
		return 0;
	case WM_SIZE:
		SetWindowPos(tab_control, NULL, 0, 0, LOWORD(lparam), HIWORD(lparam), SWP_NOMOVE);
		return 0;
	case WM_CLOSE:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static void info_window_initialize(void)
{
	WNDCLASSW wc = { 0 };

	wc.lpfnWndProc = info_window_proc;
	wc.lpszClassName = INFO_BOX_CLASS_NAME;
	wc.hCursor = LoadCursorW(NULL, IDC_ARROW);

	RUNTIME_ASSERT(RegisterClassW(&wc));

	HWND console_window = GetConsoleWindow();
	RUNTIME_ASSERT(console_window);
	RECT console_window_bounds;
	RUNTIME_ASSERT(GetWindowRect(console_window, &console_window_bounds));
	/* This will probably go out of bounds if the console is on the right side of the screen */
	int x = console_window_bounds.right + 5;
	int y = console_window_bounds.top;

	RECT info_window_bounds = { .right = INFO_BOX_CLIENT_WIDTH, .bottom = INFO_BOX_CLIENT_HEIGHT };
	RUNTIME_ASSERT(AdjustWindowRectEx(&info_window_bounds, INFO_BOX_WINDOW_STYLE, FALSE, INFO_BOX_WINDOW_STYLE_EX));
	int wx = info_window_bounds.right - info_window_bounds.left;
	int wy = info_window_bounds.bottom - info_window_bounds.top;

	/* why is passing a wide string for title wrong here? */
	window = CreateWindowExW(INFO_BOX_WINDOW_STYLE_EX, INFO_BOX_CLASS_NAME, "Dig-N-Rig Modder", INFO_BOX_WINDOW_STYLE, x, y, wx, wy, NULL, NULL, NULL, NULL);
	RUNTIME_ASSERT(window);
}

static DWORD info_thread_proc(LPVOID param)
{
	info_window_initialize();
	ShowWindow(window, SHOW_OPENWINDOW);
	SetForegroundWindow(GetConsoleWindow());
	
	DWORD result = 0;
	MSG msg = { 0 };
	while (GetMessageW(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
		if (msg.message == WM_QUIT)
		{
			result = (DWORD)msg.wParam;
		}
	}
	window = NULL;
	FreeConsole();
	/* with FreeConsole() being called here, it's not great programming, but the main method will 
		call info_destroy and clean up resources used here instead of doing it right here.
		Otherwise, I'd have to find some way to free some memory allocated in another thread */
	return result;
}

void info_initialize(info_events_t _events)
{
	current_selection_region = INVALID_REGION;
	events = _events;

	INITCOMMONCONTROLSEX icc = { .dwSize = sizeof icc, .dwICC = ICC_TREEVIEW_CLASSES | ICC_TAB_CLASSES };
	RUNTIME_ASSERT(InitCommonControlsEx(&icc));
	mineral_control_initialize();
	charmap_control_initialize();

	font_caption = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_DONTCARE, L"Arial");
	font_text = CreateFontW(-10, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_DONTCARE, L"Arial");
	RUNTIME_ASSERT(font_caption && font_text);

	thread = CreateThread(NULL, 0, info_thread_proc, NULL, 0, &thread_id);
	RUNTIME_ASSERT(thread);
	debug_format("Created thread %i for info box\n", thread_id);
}

void info_destroy(void)
{
	mineral_control_destroy();
	charmap_control_destroy();

	if (window)
	{
		PostQuitMessage(0);
	}
	if (GetConsoleWindow())
	{
		FreeConsole();
	}
	CloseHandle(thread);
	DeleteObject(font_caption);
	DeleteObject(font_text);
	UnregisterClassW(INFO_BOX_CLASS_NAME, NULL);
}

info_mode_t info_get_current_mode(void)
{
	return current_mode;
}

dnr_state_t* info_state_get(void)
{
	return state;
}

static inline HTREEITEM info_node_create(HWND tree, const LPWSTR name)
{
	TVINSERTSTRUCTW tvins;
	tvins.hParent = TVI_ROOT;
	tvins.itemex.pszText = name;
	tvins.itemex.mask = TVIF_TEXT;
	tvins.hInsertAfter = TVI_LAST;
	return TreeView_InsertItem(tree, &tvins);
}

static void info_state_set_tree_view(dnr_state_t* user_state)
{
	debug_profiler_push();
	dnr_state_t* item = user_state;

#define ADD_SERIALIZABLE(type, name) serialize_single(#type, &item->name, #name, child_windows[CWI_SAVE_TREEVIEW], NULL);
#define ADD_SERIALIZABLE_ARRAY(type, name, count) serialize_array(#type, &item->name, count, #name, child_windows[CWI_SAVE_TREEVIEW], NULL);

	SERIALIZABLE_DNR_STATE_0
	SERIALIZABLE_DNR_STATE_1
	SERIALIZABLE_DNR_STATE_2

#undef ADD_SERIALIZABLE
#undef ADD_SERIALIZABLE_ARRAY

	debug_profiler_pop("Surface level serializing");
}

void info_state_set(dnr_state_t* _state)
{
	state = _state;
		
	if (current_mode == MODE_SAVE)
	{
		/* i do not like this */
		for (int i = 0; i < 100 && !window; i++)
		{
			Sleep(10);
		}
		RUNTIME_ASSERT(window);
		SendMessageW(window, INFO_BOX_MSG_STATE_READY, (WPARAM)state, 0);
	}
}

static void info_cell_set_current_treeview()
{
	int x = current_selection_index / WORLD_HEIGHT;
	int y = current_selection_index % WORLD_HEIGHT;

	dnr_block_t* block = &state->blocks[current_selection_index];
#define ADD_SERIALIZABLE(type, name) serialize_single(#type, &item->name, #name, child_windows[CWI_SAVE_CURRENT_TREEVIEW], root);
#define ADD_SERIALIZABLE_ARRAY(type, name, count) serialize_array(#type, &item->name, count, #name, child_windows[CWI_SAVE_CURRENT_TREEVIEW], root);

	{
		HTREEITEM root = info_node_create(child_windows[CWI_SAVE_CURRENT_TREEVIEW], L"block");
		dnr_block_t* item = block;
		SERIALIZABLE_DNR_BLOCK
		TreeView_Expand(child_windows[CWI_SAVE_CURRENT_TREEVIEW], root, TVE_EXPAND);
	}
	if (block->mineral_exists)
	{
		HTREEITEM root = info_node_create(child_windows[CWI_SAVE_CURRENT_TREEVIEW], L"mineral");
		RUNTIME_ASSERT(block->mineral_index >= 0 && block->mineral_index < sizeof state->minerals / sizeof * state->minerals);
		dnr_mineral_t* item = &state->minerals[block->mineral_index];
		SERIALIZABLE_DNR_MINERAL
		TreeView_Expand(child_windows[CWI_SAVE_CURRENT_TREEVIEW], root, TVE_EXPAND);
	}
	for (int i = 0; i < state->stalactite_count; i++)
	{
		if (state->stalactite_array[i].exists && (int)state->stalactite_array[i].x == x && (int)state->stalactite_array[i].y == y)
		{
			HTREEITEM root = info_node_create(child_windows[CWI_SAVE_CURRENT_TREEVIEW], L"stalactite");
			stalactite_t* item = &state->stalactite_array[i];
			SERIALIZABLE_STALACTITE
			TreeView_Expand(child_windows[CWI_SAVE_CURRENT_TREEVIEW], root, TVE_EXPAND);
			break;
		}
	}

#undef ADD_SERIALIZABLE
#undef ADD_SERIALIZABLE_ARRAY
}

static void info_state_update_current_cell_image(int x, int y)
{
	CHAR_INFO cell = file_state_spritify_cell(state, x, y);
	int layer_index = y / TARGET_HEIGHT;
	MINERAL_CONTROL_SET_CELL(child_windows[CWI_SAVE_CURRENT_CELL], cell.Char.AsciiChar, cell.Attributes, state->layer_headers[layer_index].dirt_color);
}

void info_cell_set_current(int x, int y)
{
	if (!state)
	{
		debug_format("Tried to set current cell with no state\n");
		return;
	}

	EnableWindow(child_windows[CWI_SAVE_GO_TO_LAYER_BUTTON], FALSE);
	int pos = GetScrollPos(child_windows[CWI_SAVE_CURRENT_TREEVIEW], SB_VERT);
	TreeView_DeleteAllItems(child_windows[CWI_SAVE_CURRENT_TREEVIEW]);

	if (x == -1 && y == -1)
	{
		MINERAL_CONTROL_SET_CELL(child_windows[CWI_SAVE_CURRENT_CELL], 0, 0, 0);
		return;
	}

	RUNTIME_ASSERT(x >= 0 && y >= 0 && x < WORLD_WIDTH && y < WORLD_HEIGHT);

	info_state_update_current_cell_image(x, y);

	current_selection_region = INVALID_REGION;
	current_selection_index = x * WORLD_HEIGHT + y;
	EnableWindow(child_windows[CWI_SAVE_GO_TO_LAYER_BUTTON], TRUE);

	info_cell_set_current_treeview();
	SetScrollPos(child_windows[CWI_SAVE_CURRENT_TREEVIEW], SB_VERT, pos, TRUE);
}

static void info_cell_set_current_region_treeview(dnr_block_t* block, dnr_mineral_t* mineral)
{
#define ADD_SERIALIZABLE(type, name) serialize_single(#type, &item->name, #name, child_windows[CWI_SAVE_CURRENT_TREEVIEW], root);

	{
		HTREEITEM root = info_node_create(child_windows[CWI_SAVE_CURRENT_TREEVIEW], L"block");
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
		HTREEITEM root = info_node_create(child_windows[CWI_SAVE_CURRENT_TREEVIEW], L"mineral");
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

static int info_copy_similar_block_data(int mask, dnr_block_t* item1, const dnr_block_t* item2)
{
	HWND window = child_windows[CWI_SAVE_CURRENT_TREEVIEW];
	HTREEITEM root = serialize_tree_find_item(window, NULL, "block");
	int position = 0;

	CHECK_CONDITION(health_percentage);
	CHECK_CONDITION(health_max);
	CHECK_CONDITION(health_current);
	if (item1->visual.Char.AsciiChar != item2->visual.Char.AsciiChar || item1->visual.Attributes != item2->visual.Attributes)
	{
		HTREEITEM prev = serialize_tree_find_item(window, root, "visual");
		if (prev)
		{
			serialize_element_enable(serialize_element_get_from_node(window, prev), false);
		}
	}
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

static int info_copy_similar_mineral_data(int mask, dnr_mineral_t* item1, const dnr_mineral_t* item2)
{
	if (!item1)
	{
		return;
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
	HWND window = child_windows[CWI_SAVE_CURRENT_TREEVIEW];
	HTREEITEM root = serialize_tree_find_item(window, NULL, "mineral");
	int position = 0;

	CHECK_CONDITION(size);
	CHECK_CONDITION(type);

	return mask;
}

#undef CHECK_CONDITION

void info_cell_set_current_region(region_t region)
{
	if (!state)
	{
		debug_format("Tried to set current cell region with no state\n");
		return;
	}

	debug_profiler_push();

	region = region_validate(region);
	RUNTIME_ASSERT(!region_is_invalid(region) && dig_inside_bounds(region.x0, region.y0) && dig_inside_bounds(region.x1, region.y1));

	current_selection_index = -1;
	current_selection_region = region;

	current_block = state->blocks[region.x0 * WORLD_HEIGHT + region.y0];
	current_mineral = current_block.mineral_exists ? state->minerals[current_block.mineral_index] : (dnr_mineral_t) { 0 };

	TreeView_DeleteAllItems(child_windows[CWI_SAVE_CURRENT_TREEVIEW]);
	info_cell_set_current_region_treeview(&current_block, &current_mineral);

	int block_mask = ~0, mineral_mask = ~0;

	/* no stalactite because that's just a waste of time... */
	for (int y = region.y0; y <= region.y1; y++)
	{
		for (int x = region.x0; x <= region.x1; x++)
		{
			dnr_block_t* block = &state->blocks[x * WORLD_HEIGHT + y];
			block_mask = info_copy_similar_block_data(block_mask, &current_block, block);
			mineral_mask = info_copy_similar_mineral_data(mineral_mask, &current_mineral, block->mineral_exists ? &state->minerals[block->mineral_index] : NULL);
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

element_t info_element_find(bool global, const char* query)
{
	if (global)
	{
		return serialize_element_get_from_node(child_windows[CWI_SAVE_TREEVIEW], 
			serialize_tree_find_item(child_windows[CWI_SAVE_TREEVIEW], TVI_ROOT, query));
	}
	return serialize_element_get_from_node(child_windows[CWI_SAVE_CURRENT_TREEVIEW], 
		serialize_tree_find_item(child_windows[CWI_SAVE_CURRENT_TREEVIEW], TVI_ROOT, query));
}