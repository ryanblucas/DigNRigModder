/*
	serialize.c ~ RL
*/

#include "serialize.h"
#include "change_field_modal.h"
#include <strsafe.h>
#include <stdio.h>

struct element
{
	HWND window;
	HTREEITEM tree_item;
	uint64_t type_hash;
	void* value;
	int count;
	WCHAR name[64];
};

static inline size_t serialize_type_size(uint64_t type_hash)
{
	switch (type_hash)
	{
	case TYPE_UINT8_T:
	case TYPE_DNR_MINERAL_SIZE_T:
		return 1;
	case TYPE_UINT16_T:
	case TYPE_DNR_MINERAL_TYPE_T:
		return 2;
	case TYPE_FLOAT:
	case TYPE_BOOLEAN32_T:
	case TYPE_CHAR_INFO:
	case TYPE_INT32_T:
	case TYPE_UINT32_T:
	case TYPE_DNR_POINTER_T:
	case TYPE_DNR_RIG_TYPE_T:
	case TYPE_DNR_MINERAL_MOVE_DIRECTION_T:
	case TYPE_DNR_MINERAL_SPAWN_RULE_T:
		return 4;
	case TYPE_DNR_SPRITE_T:
		return sizeof(dnr_sprite_t);
	case TYPE_DNR_SAVE_HEADER_T:
		return sizeof(dnr_save_header_t);
	case TYPE_DNR_LAYER_HEADER_T:
		return sizeof(dnr_layer_header_t);
	case TYPE_DNR_PLAYER_T:
		return sizeof(dnr_player_t);
	case TYPE_DNR_BLOCK_T:
		return sizeof(dnr_block_t);
	case TYPE_DNR_MINERAL_T:
		return sizeof(dnr_mineral_t);
	}
	return 0;
}

/* TO DO: lots of copied and redundant code here... */

static void serialize_redo_basic(element_t element)
{
	WCHAR buf[256];
	switch (element->type_hash)
	{
	case TYPE_FLOAT:
		StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%s - %f", element->name, *(float*)element->value);
		break;
	case TYPE_BOOLEAN32_T:
	case TYPE_INT32_T:
		StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%s - %i", element->name, *(int32_t*)element->value);
		break;
	case TYPE_CHAR_INFO:
	{
		CHAR_INFO ci_value = *(CHAR_INFO*)element->value;
		StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%s - {char: %#04x, attributes: %#06x}", element->name, ci_value.Char.AsciiChar & 0xFF, ci_value.Attributes & 0xFFFF);
		break;
	}
	case TYPE_UINT32_T:
	case TYPE_DNR_POINTER_T:
		StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%s - %#010x", element->name, *(uint32_t*)element->value);
		break;
	case TYPE_UINT16_T:
		StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%s - %#06x", element->name, *(uint16_t*)element->value);
		break;
	case TYPE_UINT8_T:
		StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%s - %#04x", element->name, *(uint8_t*)element->value);
		break;

#define ADD_SERIALIZABLE_ENUM(enum_name, enum_value) case enum_value: StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%s - " L#enum_name, element->name); break;

	case TYPE_DNR_RIG_TYPE_T:
		switch (*(dnr_rig_type_t*)element->value)
		{
			SERIALIZABLE_DNR_RIG_TYPE
		default:
			StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%s - %#010x", element->name, *(dnr_rig_type_t*)element->value);
			break;
		}
		break;
	case TYPE_DNR_MINERAL_MOVE_DIRECTION_T:
		switch (*(dnr_mineral_move_direction_t*)element->value)
		{
			SERIALIZABLE_DNR_MINERAL_MOVE_DIRECTION
		default:
			StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%s - %#010x", element->name, *(dnr_mineral_move_direction_t*)element->value);
			break;
		}
		break;
	case TYPE_DNR_MINERAL_SPAWN_RULE_T:
		switch (*(dnr_mineral_spawn_rule_t*)element->value)
		{
			SERIALIZABLE_DNR_MINERAL_SPAWN_RULE
		default:
			StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%s - %#010x", element->name, *(dnr_mineral_spawn_rule_t*)element->value);
			break;
		}
		break;
	case TYPE_DNR_MINERAL_SIZE_T:
		switch (*(dnr_mineral_size_t*)element->value)
		{
			SERIALIZABLE_DNR_MINERAL_SIZE
		default:
			StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%s - %#004x", element->name, *(dnr_mineral_size_t*)element->value);
			break;
		}
		break;
	case TYPE_DNR_MINERAL_TYPE_T:
		switch (*(dnr_mineral_type_t*)element->value)
		{
			SERIALIZABLE_DNR_MINERAL_TYPE
		default:
			StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%s - %#006x", element->name, *(dnr_mineral_type_t*)element->value);
			break;
		}
		break;

#undef ADD_SERIALIZABLE_ENUM
	}

	TVITEMEX tvi = { .mask = TVIF_TEXT, .hItem = element->tree_item, .pszText = buf, .cchTextMax = sizeof buf / sizeof * buf };
	RUNTIME_ASSERT(TreeView_SetItem(element->window, &tvi));
}

static void serialize_array_internal(uint64_t type_hash, void* value, int start, int end, WCHAR* wname, HWND tree_window, HTREEITEM tree_item)
{
	value = (uint8_t*)value + serialize_type_size(type_hash) * (end - 1);
	for (int i = end - 1; i >= start; i--)
	{
		TVINSERTSTRUCTW tvins = { 0 };
		WCHAR buf[256];
		StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%i", i);
		tvins.itemex.pszText = buf;
		tvins.itemex.cchTextMax = sizeof buf / sizeof * buf;
		tvins.itemex.mask = TVIF_TEXT;

		tvins.hParent = tree_item;
		tvins.hInsertAfter = TVI_FIRST;

		switch (type_hash)
		{
		case TYPE_FLOAT:
			StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%i - %f", i, *(float*)value);
			value = (float*)value - 1;
			break;
		case TYPE_BOOLEAN32_T:
		case TYPE_INT32_T:
			StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%i - %i", i, *(int32_t*)value);
			value = (int32_t*)value - 1;
			break;
		case TYPE_CHAR_INFO:
		{
			CHAR_INFO ci_value = *(CHAR_INFO*)value;
			StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%i - {char: %#04x, attributes: %#06x}", i, ci_value.Char.AsciiChar & 0xFF, ci_value.Attributes & 0xFFFF);
			value = (CHAR_INFO*)value - 1;
			break;
		}
		case TYPE_UINT32_T:
		case TYPE_DNR_POINTER_T:
			StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%i - %#010x", i, *(uint32_t*)value);
			value = (uint32_t*)value - 1;
			break;
		case TYPE_UINT16_T:
			StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%i - %#06x", i, *(uint16_t*)value);
			value = (uint16_t*)value - 1;
			break;
		case TYPE_UINT8_T:
			StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%i - %#04x", i, *(uint8_t*)value);
			value = (uint8_t*)value - 1;
			break;

#define ADD_SERIALIZABLE_ENUM(enum_name, enum_value) case enum_value: StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%i - " L#enum_name, i); break;

		case TYPE_DNR_RIG_TYPE_T:
			switch (*(dnr_rig_type_t*)value)
			{
				SERIALIZABLE_DNR_RIG_TYPE
			default:
				StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%i - %#010x", i, *(dnr_rig_type_t*)value);
				break;
			}
			value = (dnr_rig_type_t*)value - 1;
			break;
		case TYPE_DNR_MINERAL_MOVE_DIRECTION_T:
			switch (*(dnr_mineral_move_direction_t*)value)
			{
				SERIALIZABLE_DNR_MINERAL_MOVE_DIRECTION
			default:
				StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%i - %#010x", i, *(dnr_mineral_move_direction_t*)value);
				break;
			}
			value = (dnr_mineral_move_direction_t*)value - 1;
			break;
		case TYPE_DNR_MINERAL_SPAWN_RULE_T:
			switch (*(dnr_mineral_spawn_rule_t*)value)
			{
				SERIALIZABLE_DNR_MINERAL_SPAWN_RULE
			default:
				StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%i - %#010x", i, *(dnr_mineral_spawn_rule_t*)value);
				break;
			}
			value = (dnr_mineral_spawn_rule_t*)value - 1;
			break;
		case TYPE_DNR_MINERAL_SIZE_T:
			switch (*(dnr_mineral_size_t*)value)
			{
				SERIALIZABLE_DNR_MINERAL_SIZE
			default:
				StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%i - %#004x", i, *(dnr_mineral_size_t*)value);
				break;
			}
			value = (dnr_mineral_type_t*)value - 1;
			break;
		case TYPE_DNR_MINERAL_TYPE_T:
			switch (*(dnr_mineral_type_t*)value)
			{
				SERIALIZABLE_DNR_MINERAL_TYPE
			default:
				StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%i - %#006x", i, *(dnr_mineral_type_t*)value);
				break;
			}
			value = (dnr_mineral_type_t*)value - 1;
			break;

#undef ADD_SERIALIZABLE_ENUM

#define ADD_SERIALIZABLE(type, name) serialize_single(#type, &item->name, #name, tree_window, next_tree_item);
#define ADD_SERIALIZABLE_ARRAY(type, name, count) serialize_array(#type, &item->name, count, #name, tree_window, next_tree_item);

		case TYPE_DNR_SPRITE_T:
		{
			dnr_sprite_t* item = (dnr_sprite_t*)value;
			HTREEITEM next_tree_item = TreeView_InsertItem(tree_window, &tvins);
			SERIALIZABLE_DNR_SPRITE
			value = item - 1;
			break;
		}

		case TYPE_DNR_SAVE_HEADER_T:
		{
			dnr_save_header_t* item = (dnr_save_header_t*)value;
			HTREEITEM next_tree_item = TreeView_InsertItem(tree_window, &tvins);
			SERIALIZABLE_DNR_SAVE_HEADER
			value = item - 1;
			continue;
		}

		case TYPE_DNR_LAYER_HEADER_T:
		{
			dnr_layer_header_t* item = (dnr_layer_header_t*)value;
			HTREEITEM next_tree_item = TreeView_InsertItem(tree_window, &tvins);
			SERIALIZABLE_DNR_LAYER_HEADER
			value = item - 1;
			continue;
		}

		case TYPE_DNR_PLAYER_T:
		{
			dnr_player_t* item = (dnr_player_t*)value;
			HTREEITEM next_tree_item = TreeView_InsertItem(tree_window, &tvins);
			SERIALIZABLE_DNR_PLAYER
			value = item - 1;
			continue;
		}

		case TYPE_DNR_BLOCK_T:
		{
			dnr_block_t* item = (dnr_block_t*)value;
			tvins.itemex.mask |= TVIF_PARAM;
			element_t se = dig_malloc(sizeof * se);
			*se = (struct element){ .type_hash = type_hash, .value = value, .count = 0 };
			wcsncpy(se->name, tvins.itemex.pszText, sizeof se->name / sizeof * se->name - 1);
			tvins.itemex.lParam = (LPARAM)se;

			HTREEITEM next_tree_item = TreeView_InsertItem(tree_window, &tvins);

			tvins.itemex.mask = 0;
			tvins.hParent = next_tree_item;
			tvins.hInsertAfter = TVI_FIRST;

			TreeView_InsertItem(tree_window, &tvins);

			value = item - 1;
			continue;
		}

		case TYPE_DNR_MINERAL_T:
		{
			dnr_mineral_t* item = (dnr_mineral_t*)value;
			tvins.itemex.mask |= TVIF_PARAM;
			element_t se = dig_malloc(sizeof * se);
			*se = (struct element){ .type_hash = type_hash, .value = value, .count = 0 };
			wcsncpy(se->name, tvins.itemex.pszText, sizeof se->name / sizeof * se->name - 1);
			tvins.itemex.lParam = (LPARAM)se;

			HTREEITEM next_tree_item = TreeView_InsertItem(tree_window, &tvins);

			tvins.itemex.mask = 0;
			tvins.hParent = next_tree_item;
			tvins.hInsertAfter = TVI_FIRST;

			TreeView_InsertItem(tree_window, &tvins);

			value = item - 1;
			continue;
		}

#undef ADD_SERIALIZABLE
#undef ADD_SERIALIZABLE_ARRAY
		default:
			RUNTIME_ASSERT(false);
		}

		HTREEITEM item = TreeView_InsertItem(tree_window, &tvins);
		RUNTIME_ASSERT(item);
	}
}

static HTREEITEM serialize_single_internal(bool post_populate, uint64_t type_hash, void* value, WCHAR* wname, HWND tree_window, HTREEITEM tree_item)
{
	TVINSERTSTRUCTW tvins = { 0 };

	WCHAR buf[256];
	tvins.itemex.pszText = buf;
	tvins.itemex.cchTextMax = sizeof buf / sizeof * buf;
	tvins.itemex.mask = TVIF_TEXT;

	tvins.hParent = tree_item;
	tvins.hInsertAfter = TVI_LAST;

	switch (type_hash)
	{
	case TYPE_FLOAT:
		StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%s - %f", wname, *(float*)value);
		break;
	case TYPE_BOOLEAN32_T:
	case TYPE_INT32_T:
		StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%s - %i", wname, *(int32_t*)value);
		break;
	case TYPE_CHAR_INFO:
	{
		CHAR_INFO ci_value = *(CHAR_INFO*)value;
		StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%s - {char: %#04x, attributes: %#06x}", wname, ci_value.Char.AsciiChar & 0xFF, ci_value.Attributes & 0xFFFF);
		break;
	}
	case TYPE_UINT32_T:
	case TYPE_DNR_POINTER_T:
		StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%s - %#010x", wname, *(uint32_t*)value);
		break;
	case TYPE_UINT16_T:
		StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%s - %#06x", wname, *(uint16_t*)value);
		break;
	case TYPE_UINT8_T:
		StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%s - %#04x", wname, *(uint8_t*)value);
		break;

#define ADD_SERIALIZABLE_ENUM(enum_name, enum_value) case enum_value: StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%s - " L#enum_name, wname); break;

	case TYPE_DNR_RIG_TYPE_T:
		switch (*(dnr_rig_type_t*)value)
		{
			SERIALIZABLE_DNR_RIG_TYPE
		default:
			StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%s - %#010x", wname, *(dnr_rig_type_t*)value);
			break;
		}
		break;
	case TYPE_DNR_MINERAL_MOVE_DIRECTION_T:
		switch (*(dnr_mineral_move_direction_t*)value)
		{
			SERIALIZABLE_DNR_MINERAL_MOVE_DIRECTION
		default:
			StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%s - %#010x", wname, *(dnr_mineral_move_direction_t*)value);
			break;
		}
		break;
	case TYPE_DNR_MINERAL_SPAWN_RULE_T:
		switch (*(dnr_mineral_spawn_rule_t*)value)
		{
			SERIALIZABLE_DNR_MINERAL_SPAWN_RULE
		default:
			StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%s - %#010x", wname, *(dnr_mineral_spawn_rule_t*)value);
			break;
		}
		break;
	case TYPE_DNR_MINERAL_SIZE_T:
		switch (*(dnr_mineral_size_t*)value)
		{
			SERIALIZABLE_DNR_MINERAL_SIZE
		default:
			StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%s - %#004x", wname, *(dnr_mineral_size_t*)value);
			break;
		}
		break;
	case TYPE_DNR_MINERAL_TYPE_T:
		switch (*(dnr_mineral_type_t*)value)
		{
			SERIALIZABLE_DNR_MINERAL_TYPE
		default:
			StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%s - %#006x", wname, *(dnr_mineral_type_t*)value);
			break;
		}
		break;

#undef ADD_SERIALIZABLE_ENUM

#define ADD_SERIALIZABLE(type, name) serialize_single(#type, &item->name, #name, tree_window, next_tree_item);
#define ADD_SERIALIZABLE_ARRAY(type, name, count) serialize_array(#type, &item->name, count, #name, tree_window, next_tree_item);

	case TYPE_DNR_SPRITE_T:
	{
		dnr_sprite_t* item = (dnr_sprite_t*)value;
		tvins.itemex.pszText = wname;
		tvins.itemex.cchTextMax = 0;
		HTREEITEM next_tree_item = TreeView_InsertItem(tree_window, &tvins);
		SERIALIZABLE_DNR_SPRITE
		return next_tree_item;
	}

	case TYPE_DNR_SAVE_HEADER_T:
	{
		dnr_save_header_t* item = (dnr_save_header_t*)value;
		tvins.itemex.pszText = wname;
		tvins.itemex.cchTextMax = 0;
		HTREEITEM next_tree_item = TreeView_InsertItem(tree_window, &tvins);
		SERIALIZABLE_DNR_SAVE_HEADER
		return next_tree_item;
	}

	case TYPE_DNR_LAYER_HEADER_T:
	{
		dnr_layer_header_t* item = (dnr_layer_header_t*)value;
		tvins.itemex.pszText = wname;
		tvins.itemex.cchTextMax = 0;
		HTREEITEM next_tree_item = TreeView_InsertItem(tree_window, &tvins);
		SERIALIZABLE_DNR_LAYER_HEADER
		return next_tree_item;
	}

	case TYPE_DNR_PLAYER_T:
	{
		dnr_player_t* item = (dnr_player_t*)value;
		tvins.itemex.pszText = wname;
		tvins.itemex.cchTextMax = 0;
		HTREEITEM next_tree_item = TreeView_InsertItem(tree_window, &tvins);
		SERIALIZABLE_DNR_PLAYER
		return next_tree_item;
	}

	case TYPE_DNR_BLOCK_T:
	{
		dnr_block_t* item = (dnr_block_t*)value;
		HTREEITEM next_tree_item = tree_item;
		if (!post_populate)
		{
			tvins.itemex.pszText = wname;
			tvins.itemex.cchTextMax = 0;
			next_tree_item = TreeView_InsertItem(tree_window, &tvins);
		}
		SERIALIZABLE_DNR_BLOCK
		return next_tree_item;
	}

	case TYPE_DNR_MINERAL_T:
	{
		dnr_mineral_t* item = (dnr_mineral_t*)value;
		HTREEITEM next_tree_item = tree_item;
		if (!post_populate)
		{
			tvins.itemex.pszText = wname;
			tvins.itemex.cchTextMax = 0;
			next_tree_item = TreeView_InsertItem(tree_window, &tvins);
		}
		SERIALIZABLE_DNR_MINERAL
		return next_tree_item;
	}

#undef ADD_SERIALIZABLE
#undef ADD_SERIALIZABLE_ARRAY
	default:
		RUNTIME_ASSERT(false);
	}

	HTREEITEM item = TreeView_InsertItem(tree_window, &tvins);
	RUNTIME_ASSERT(item);
	return item;
}

static inline uint64_t serialize_hash(const char* str)
{
	uint64_t hash = 5381;
	do
	{
		hash = ((hash << 5) + hash) ^ *str;
	} while (*++str);
	return hash;
}

void serialize_single(const char* type, void* value, const char* name, HWND tree_window, HTREEITEM tree_item)
{
	uint64_t type_hash = serialize_hash(type);
	WCHAR wname[64];
	RUNTIME_ASSERT(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, name, -1, wname, sizeof wname / sizeof * wname));

	tree_item = serialize_single_internal(false, type_hash, value, wname, tree_window, tree_item);

	element_t se = dig_malloc(sizeof * se);
	*se = (struct element){ .window = tree_window, .tree_item = tree_item, .type_hash = type_hash, .value = value, .count = 0 };
	wcsncpy(se->name, wname, sizeof se->name / sizeof * se->name - 1);

	TVITEMEXW tvi = { .mask = TVIF_PARAM, .hItem = tree_item, .lParam = (LPARAM)se };
	TreeView_SetItem(tree_window, &tvi);
}

void serialize_array(const char* type, void* value, int count, const char* name, HWND tree_window, HTREEITEM tree_item)
{
	uint64_t type_hash = serialize_hash(type);
	WCHAR wname[64];
	RUNTIME_ASSERT(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, name, -1, wname, sizeof wname / sizeof * wname));

	TVINSERTSTRUCTW tvins = { 0 };

	WCHAR buf[256];
	StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%s - %i", wname, count);
	tvins.itemex.pszText = buf;
	tvins.itemex.cchTextMax = 0;
	tvins.itemex.mask = TVIF_TEXT;

	tvins.hParent = tree_item;
	tvins.hInsertAfter = TVI_LAST;

	tvins.itemex.mask |= TVIF_PARAM;
	element_t se = dig_malloc(sizeof * se);
	*se = (struct element) { .window = tree_window, .type_hash = type_hash, .value = value, .count = count };
	wcsncpy(se->name, wname, sizeof se->name / sizeof * se->name - 1);
	tvins.itemex.lParam = (LPARAM)se;

	tree_item = TreeView_InsertItem(tree_window, &tvins);
	se->tree_item = tree_item;
	serialize_array_internal(type_hash, value, 0, 1, wname, tree_window, tree_item);
}

void serialize_delete_internal(HWND tree_window, HTREEITEM item)
{
	if (!item)
	{
		return;
	}
	HTREEITEM curr = TreeView_GetChild(tree_window, item);
	do
	{
		serialize_delete_internal(tree_window, TreeView_GetChild(tree_window, item));
		TVITEMEXW item = { .mask = TVIF_PARAM };
		TreeView_GetItem(tree_window, &item);
		free((void*)item.lParam);
	} while (curr = TreeView_GetNextSibling(tree_window, curr));
}

void serialize_delete(HWND tree_window)
{
	serialize_delete_internal(tree_window, TreeView_GetRoot(tree_window));
	TreeView_DeleteAllItems(tree_window);
}

void serialize_on_expand(element_t element)
{
	if (!element || element->count == -1)
	{
		return;
	}

	debug_profiler_push();

	char buf[128];
	int res = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, element->name, -1, buf, sizeof buf, NULL, NULL);

	TreeView_DeleteItem(element->window, TreeView_GetChild(element->window, element->tree_item));
	if (element->count == 0)
	{
		serialize_single_internal(true, element->type_hash, element->value, element->name, element->window, element->tree_item);
		element->count = -1;
	}
	else
	{
		serialize_array_internal(element->type_hash, element->value, 0, element->count, element->name, element->window, element->tree_item);
		element->count = -1;
	}

	if (res)
	{
		debug_profiler_pop("Serializing %s", buf);
	}
	else
	{
		debug_profiler_pop("Post-serializing");
	}
}

void serialize_on_change_field(element_t element)
{
	HWND owner = GetParent(element->window);
	RUNTIME_ASSERT(owner);
	switch (element->type_hash)
	{
	case TYPE_DNR_MINERAL_SIZE_T:
		change_field_modal_mineral_size(owner, element->value);
		break;
	case TYPE_DNR_MINERAL_TYPE_T:
		change_field_modal_mineral_type(owner, element->value);
		break;
	case TYPE_DNR_RIG_TYPE_T:
		change_field_modal_rig_type(owner, element->value);
		break;
	case TYPE_DNR_MINERAL_MOVE_DIRECTION_T:
		change_field_modal_mineral_move_direction(owner, element->value);
		break;
	case TYPE_DNR_MINERAL_SPAWN_RULE_T:
		change_field_modal_mineral_spawn_rule(owner, element->value);
		break;
	case TYPE_UINT8_T:
		change_field_modal_integer(owner, element->value, sizeof(uint8_t));
		break;
	case TYPE_UINT16_T:
		change_field_modal_integer(owner, element->value, sizeof(uint16_t));
		break;
	case TYPE_INT32_T:
		change_field_modal_integer(owner, element->value, sizeof(int32_t) | SIZE_IS_SIGNED);
		break;
	case TYPE_UINT32_T:
		change_field_modal_integer(owner, element->value, sizeof(uint32_t));
		break;
	case TYPE_BOOLEAN32_T:
		change_field_modal_integer(owner, element->value, sizeof(boolean32_t) | SIZE_IS_SIGNED);
		break;
	case TYPE_FLOAT:
		change_field_modal_float(owner, element->value);
		break;
	case TYPE_CHAR_INFO:
		change_field_modal_char_info(owner, element->value);
		break;
	}
	serialize_redo_basic(element);
}

HTREEITEM serialize_tree_find_item(HWND tree_window, HTREEITEM root, const char* name)
{
	HTREEITEM curr = TreeView_GetChild(tree_window, root);
	while (curr)
	{
		WCHAR wname[64], tname[64] = { 0 };
		TVITEMEX tvix = { .mask = TVIF_TEXT, .pszText = tname, .cchTextMax = sizeof tname / sizeof * tname, .hItem = curr };
		TreeView_GetItem(tree_window, &tvix);

		RUNTIME_ASSERT(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, name, -1, wname, sizeof wname / sizeof * wname));
		
		if (!tvix.pszText)
		{
			continue;
		}

		if (wcsncmp(wname, tvix.pszText, sizeof wname) == 0)
		{
			return curr;
		}

		curr = TreeView_GetNextSibling(tree_window, curr);
	}
	return curr;
}