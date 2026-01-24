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
		StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%s - %f\n", wname, *(float*)value);
		break;
	case TYPE_BOOLEAN32_T:
	case TYPE_INT32_T:
		StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%s - %i\n", wname, *(int32_t*)value);
		break;
	case TYPE_CHAR_INFO:
	{
		CHAR_INFO ci_value = *(CHAR_INFO*)value;
		StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%s - {char: %#04x, attributes: %#06x}\n", wname, ci_value.Char.AsciiChar, ci_value.Attributes);
		break;
	}
	case TYPE_UINT32_T:
	case TYPE_DNR_POINTER_T:
		StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%s - %#010x\n", wname, *(uint32_t*)value);
		break;
	case TYPE_UINT16_T:
		StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%s - %#06x\n", wname, *(uint16_t*)value);
		break;
	case TYPE_UINT8_T:
		StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%s - %#04x\n", wname, *(uint8_t*)value);
		break;
	case TYPE_DNR_RIG_TYPE_T:
#define ADD_SERIALIZABLE_ENUM(enum_name, enum_value) case enum_value: StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%s - " L#enum_name "\n", wname); break;
		switch (*(dnr_rig_type_t*)value)
		{
			SERIALIZABLE_DNR_RIG_TYPE
		default:
			StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%s - %#010x\n", wname, *(dnr_rig_type_t*)value);
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
		tvins.itemex.pszText = wname;
		tvins.itemex.cchTextMax = 0;
		HTREEITEM next_tree_item = TreeView_InsertItem(tree_window, &tvins);
		SERIALIZABLE_DNR_BLOCK
		return;
	}

	case TYPE_DNR_MINERAL_T:
	{
		dnr_mineral_t* item = (dnr_mineral_t*)value;
		tvins.itemex.pszText = wname;
		tvins.itemex.cchTextMax = 0;
		HTREEITEM next_tree_item = TreeView_InsertItem(tree_window, &tvins);
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

void serialize_array(const char* type, void* value, int count, const char* name, HWND tree_window, HTREEITEM tree_item)
{
	uint64_t type_hash = serialize_hash(type);
	WCHAR wname[64];
	RUNTIME_ASSERT(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, name, -1, wname, sizeof wname / sizeof * wname));

	{
		TVINSERTSTRUCTW tvins = { 0 };

		WCHAR buf[256];
		StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%s - %i\n", wname, count);
		tvins.itemex.pszText = buf;
		tvins.itemex.cchTextMax = 0;
		tvins.itemex.mask = TVIF_TEXT;

		tvins.hParent = tree_item;
		tvins.hInsertAfter = TVI_LAST;

		tree_item = TreeView_InsertItem(tree_window, &tvins);
	}

	for (int i = 0; i < count; i++)
	{
		TVINSERTSTRUCTW tvins = { 0 };

		WCHAR buf[256];
		StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%i\n", i);
		tvins.itemex.pszText = buf;
		tvins.itemex.cchTextMax = sizeof buf / sizeof * buf;
		tvins.itemex.mask = TVIF_TEXT;

		tvins.hParent = tree_item;
		tvins.hInsertAfter = TVI_LAST;

		switch (type_hash)
		{
		case TYPE_FLOAT:
			StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%i - %f\n", i, *(float*)value);
			value = (float*)value + 1;
			break;
		case TYPE_BOOLEAN32_T:
		case TYPE_INT32_T:
			StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%i - %i\n", i, *(int32_t*)value);
			value = (int32_t*)value + 1;
			break;
		case TYPE_CHAR_INFO:
		{
			CHAR_INFO ci_value = *(CHAR_INFO*)value;
			StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%i - {char: %#04x, attributes: %#06x}\n", i, ci_value.Char.AsciiChar, ci_value.Attributes);
			value = (CHAR_INFO*)value + 1;
			break;
		}
		case TYPE_UINT32_T:
		case TYPE_DNR_POINTER_T:
			StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%i - %#010x\n", i, *(uint32_t*)value);
			value = (uint32_t*)value + 1;
			break;
		case TYPE_UINT16_T:
			StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%s - %#06x\n", wname, *(uint16_t*)value);
			value = (uint16_t*)value + 1;
			break;
		case TYPE_UINT8_T:
			StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%i - %#04x\n", i, *(uint8_t*)value);
			value = (uint8_t*)value + 1;
			break;
		case TYPE_DNR_RIG_TYPE_T:
#define ADD_SERIALIZABLE_ENUM(enum_name, enum_value) case enum_value: StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%i - " L#enum_name "\n", i); break;
			switch (*(dnr_rig_type_t*)value)
			{
				SERIALIZABLE_DNR_RIG_TYPE
			default:
				StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%s - %#010x\n", wname, *(dnr_rig_type_t*)value);
				break;
			}
#undef ADD_SERIALIZABLE_ENUM
			value = (dnr_rig_type_t*)value + 1;
			break;

#define ADD_SERIALIZABLE(type, name) serialize_single(#type, &item->name, #name, tree_window, next_tree_item);
#define ADD_SERIALIZABLE_ARRAY(type, name, count) serialize_array(#type, &item->name, count, #name, tree_window, next_tree_item);

		case TYPE_DNR_SPRITE_T:
		{
			dnr_sprite_t* item = (dnr_sprite_t*)value;
			HTREEITEM next_tree_item = TreeView_InsertItem(tree_window, &tvins);
			SERIALIZABLE_DNR_SPRITE
			value = item + 1;
			break;
		}

		case TYPE_DNR_SAVE_HEADER_T:
		{
			dnr_save_header_t* item = (dnr_save_header_t*)value;
			HTREEITEM next_tree_item = TreeView_InsertItem(tree_window, &tvins);
			SERIALIZABLE_DNR_SAVE_HEADER
			value = item + 1;
			continue;
		}

		case TYPE_DNR_LAYER_HEADER_T:
		{
			dnr_layer_header_t* item = (dnr_layer_header_t*)value;
			HTREEITEM next_tree_item = TreeView_InsertItem(tree_window, &tvins);
			SERIALIZABLE_DNR_LAYER_HEADER
			value = item + 1;
			continue;
		}

		case TYPE_DNR_PLAYER_T:
		{
			dnr_player_t* item = (dnr_player_t*)value;
			HTREEITEM next_tree_item = TreeView_InsertItem(tree_window, &tvins);
			SERIALIZABLE_DNR_PLAYER
			value = item + 1;
			continue;
		}

		case TYPE_DNR_BLOCK_T:
		{
			dnr_block_t* item = (dnr_block_t*)value;
			HTREEITEM next_tree_item = TreeView_InsertItem(tree_window, &tvins);
			SERIALIZABLE_DNR_BLOCK
			value = item + 1;
			continue;
		}

		case TYPE_DNR_MINERAL_T:
		{
			dnr_mineral_t* item = (dnr_mineral_t*)value;
			HTREEITEM next_tree_item = TreeView_InsertItem(tree_window, &tvins);
			SERIALIZABLE_DNR_MINERAL
			value = item + 1;
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