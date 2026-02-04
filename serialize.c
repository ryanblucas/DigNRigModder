/*
	serialize.c ~ RL
*/

#include "serialize.h"
#include <strsafe.h>
#include <stdio.h>

/* by running serialize_hash on each of the types as a string, you get this result. */

#define TYPE_FLOAT 210624726069ULL
#define TYPE_BOOLEAN32_T 13766221191973021547ULL
#define TYPE_CHAR_INFO 249834764690065676ULL
#define TYPE_INT32_T 229378475688636ULL
#define TYPE_UINT32_T 7569686425136137ULL
#define TYPE_DNR_POINTER_T 15207900801384124882ULL
#define TYPE_UINT16_T 7569686425208015ULL
#define TYPE_UINT8_T 229384437135984ULL
#define TYPE_DNR_SPRITE_T 11081697800589051136ULL
#define TYPE_DNR_RIG_TYPE_T 3798355938377845426ULL
#define TYPE_DNR_SAVE_HEADER_T 12164599792533459048ULL
#define TYPE_DNR_LAYER_HEADER_T 663611519707857130ULL
#define TYPE_DNR_PLAYER_T 11081698126627463866ULL
#define TYPE_DNR_BLOCK_T 13751622877662047520ULL
#define TYPE_DNR_MINERAL_T 15207893016492402041ULL

static bool is_surface;

bool serialize_is_surface_mode(void)
{
	return is_surface;
}

void serialize_set_preview_mode(bool mode)
{
	is_surface = mode;
}

static inline size_t serialize_type_size(uint64_t type_hash)
{
	switch (type_hash)
	{
	case TYPE_UINT8_T:
		return 1;
	case TYPE_UINT16_T:
		return 2;
	case TYPE_FLOAT:
	case TYPE_BOOLEAN32_T:
	case TYPE_CHAR_INFO:
	case TYPE_INT32_T:
	case TYPE_UINT32_T:
	case TYPE_DNR_POINTER_T:
	case TYPE_DNR_RIG_TYPE_T:
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
			StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%i - {char: %#04x, attributes: %#06x}", i, ci_value.Char.AsciiChar, ci_value.Attributes);
			value = (CHAR_INFO*)value - 1;
			break;
		}
		case TYPE_UINT32_T:
		case TYPE_DNR_POINTER_T:
			StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%i - %#010x", i, *(uint32_t*)value);
			value = (uint32_t*)value - 1;
			break;
		case TYPE_UINT16_T:
			StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%s - %#06x", wname, *(uint16_t*)value);
			value = (uint16_t*)value - 1;
			break;
		case TYPE_UINT8_T:
			StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%i - %#04x", i, *(uint8_t*)value);
			value = (uint8_t*)value - 1;
			break;
		case TYPE_DNR_RIG_TYPE_T:
#define ADD_SERIALIZABLE_ENUM(enum_name, enum_value) case enum_value: StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%i - " L#enum_name, i); break;
			switch (*(dnr_rig_type_t*)value)
			{
				SERIALIZABLE_DNR_RIG_TYPE
			default:
				StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%s - %#010x", wname, *(dnr_rig_type_t*)value);
				break;
			}
#undef ADD_SERIALIZABLE_ENUM
			value = (dnr_rig_type_t*)value - 1;
			break;

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
			if (is_surface)
			{
				tvins.itemex.mask |= TVIF_PARAM;
				surface_element_t* se = dig_malloc(sizeof * se);
				*se = (surface_element_t){ .type_hash = type_hash, .value = value, .count = 0 };
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

			HTREEITEM next_tree_item = TreeView_InsertItem(tree_window, &tvins);
			SERIALIZABLE_DNR_BLOCK
			value = item - 1;
			continue;
		}

		case TYPE_DNR_MINERAL_T:
		{
			dnr_mineral_t* item = (dnr_mineral_t*)value;
			if (is_surface)
			{
				tvins.itemex.mask |= TVIF_PARAM;
				surface_element_t* se = dig_malloc(sizeof * se);
				*se = (surface_element_t){ .type_hash = type_hash, .value = value, .count = 0 };
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

			HTREEITEM next_tree_item = TreeView_InsertItem(tree_window, &tvins);
			SERIALIZABLE_DNR_MINERAL
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

static void serialize_single_internal(bool post_populate, uint64_t type_hash, void* value, WCHAR* wname, HWND tree_window, HTREEITEM tree_item)
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
		StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%s - {char: %#04x, attributes: %#06x}", wname, ci_value.Char.AsciiChar, ci_value.Attributes);
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
	case TYPE_DNR_RIG_TYPE_T:
#define ADD_SERIALIZABLE_ENUM(enum_name, enum_value) case enum_value: StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%s - " L#enum_name, wname); break;
		switch (*(dnr_rig_type_t*)value)
		{
			SERIALIZABLE_DNR_RIG_TYPE
		default:
			StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%s - %#010x", wname, *(dnr_rig_type_t*)value);
			break;
		}
#undef ADD_SERIALIZABLE_ENUM
		break;

#define ADD_SERIALIZABLE(type, name) serialize_single(#type, &item->name, #name, tree_window, next_tree_item);
#define ADD_SERIALIZABLE_ARRAY(type, name, count) serialize_array(#type, &item->name, count, #name, tree_window, next_tree_item);

	case TYPE_DNR_SPRITE_T:
	{
		dnr_sprite_t* item = (dnr_sprite_t*)value;
		tvins.itemex.pszText = wname;
		tvins.itemex.cchTextMax = 0;
		HTREEITEM next_tree_item = TreeView_InsertItem(tree_window, &tvins);
		SERIALIZABLE_DNR_SPRITE
		return;
	}

	case TYPE_DNR_SAVE_HEADER_T:
	{
		dnr_save_header_t* item = (dnr_save_header_t*)value;
		tvins.itemex.pszText = wname;
		tvins.itemex.cchTextMax = 0;
		HTREEITEM next_tree_item = TreeView_InsertItem(tree_window, &tvins);
		SERIALIZABLE_DNR_SAVE_HEADER
		return;
	}

	case TYPE_DNR_LAYER_HEADER_T:
	{
		dnr_layer_header_t* item = (dnr_layer_header_t*)value;
		tvins.itemex.pszText = wname;
		tvins.itemex.cchTextMax = 0;
		HTREEITEM next_tree_item = TreeView_InsertItem(tree_window, &tvins);
		SERIALIZABLE_DNR_LAYER_HEADER
		return;
	}

	case TYPE_DNR_PLAYER_T:
	{
		dnr_player_t* item = (dnr_player_t*)value;
		tvins.itemex.pszText = wname;
		tvins.itemex.cchTextMax = 0;
		HTREEITEM next_tree_item = TreeView_InsertItem(tree_window, &tvins);
		SERIALIZABLE_DNR_PLAYER
		return;
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
		return;
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
		return;
	}

#undef ADD_SERIALIZABLE
#undef ADD_SERIALIZABLE_ARRAY
	default:
		RUNTIME_ASSERT(false);
	}

	HTREEITEM item = TreeView_InsertItem(tree_window, &tvins);
	RUNTIME_ASSERT(item);
}

void serialize_finalize(surface_element_t* se, HWND tree_window, HTREEITEM tree_item)
{
	if (se)
	{
		TreeView_DeleteItem(tree_window, TreeView_GetChild(tree_window, tree_item));
		if (se->count == 0)
		{
			serialize_single_internal(true, se->type_hash, se->value, se->name, tree_window, tree_item);
		}
		else
		{
			serialize_array_internal(se->type_hash, se->value, 0, se->count, se->name, tree_window, tree_item);
		}
		free(se);
	}
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
	serialize_single_internal(false, type_hash, value, wname, tree_window, tree_item);
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

	int loop_count = count;
	if (is_surface)
	{
		loop_count = 1;
		tvins.itemex.mask |= TVIF_PARAM;
		surface_element_t* se = dig_malloc(sizeof * se);
		*se = (surface_element_t) { .type_hash = type_hash, .value = value, .count = count };
		wcsncpy(se->name, wname, sizeof se->name / sizeof * se->name - 1);
		tvins.itemex.lParam = (LPARAM)se;
	}

	tree_item = TreeView_InsertItem(tree_window, &tvins);
	serialize_array_internal(type_hash, value, 0, loop_count, wname, tree_window, tree_item);
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

HTREEITEM serialize_tree_find_item(HWND tree_window, HTREEITEM root, const char* name)
{
	HTREEITEM curr = TreeView_GetChild(tree_window, root);
	while (curr)
	{
		WCHAR wname[64], tname[64];
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