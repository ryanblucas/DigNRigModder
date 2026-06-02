/*
	change_field_modal.c ~ RL
*/

#include "change_field_modal.h"
#include "charmap_control.h"
#include "../file.h"
#include "mineral_control.h"
#include "resource.h"
#include <strsafe.h>
#include <windowsx.h>

struct modal_open_struct
{
	const WCHAR* title;
	int choice_first, choice_length;
	WCHAR* choice_array;
	WCHAR result[32];
};

struct modal_charinfo_open_struct
{
	const WCHAR* title;
	CHAR_INFO* value;
	CHAR_INFO* curr_value;
};

typedef int (*conversion_func)(int);

static int cfm_mineral_type_to_index(int type)
{
	switch (type)
	{
	case MINERAL_TYPE_METALOID:
		return 0;
	case MINERAL_TYPE_ALUMON:
		return 1;
	case MINERAL_TYPE_EXPLODIUM:
		return 2;
	case MINERAL_TYPE_ENERGITE:
		return 3;
	case MINERAL_TYPE_RADIOACTON:
		return 4;
	case MINERAL_TYPE_ATOMICIDE:
		return 5;
	case MINERAL_TYPE_SCIPHIDE:
		return 6;
	case MINERAL_TYPE_RAREIEST:
		return 7;
	}
	return 0;
}

static int cfm_index_to_mineral_type(int index)
{
	switch (index)
	{
	case 0:
		return MINERAL_TYPE_METALOID;
	case 1:
		return MINERAL_TYPE_ALUMON;
	case 2:
		return MINERAL_TYPE_EXPLODIUM;
	case 3:
		return MINERAL_TYPE_ENERGITE;
	case 4:
		return MINERAL_TYPE_RADIOACTON;
	case 5:
		return MINERAL_TYPE_ATOMICIDE;
	case 6:
		return MINERAL_TYPE_SCIPHIDE;
	case 7:
		return MINERAL_TYPE_RAREIEST;
	}
	return MINERAL_TYPE_METALOID;
}

static int cfm_rig_type_to_index(int type)
{
	switch (type)
	{
	case RIG_NONE:
	case RIG_LADDER_CENTER:
	case RIG_LADDER:
		return type;
	case RIG_SCOOPER:
		return type - 1;
	case RIG_PLATFORM:
	case RIG_DIRT:
	case RIG_STONE:
	case RIG_BRICK:
	case RIG_LAVA:
	case RIG_WATER:
	case RIG_CONVEYOR_LEFT:
	case RIG_CONVEYOR_RIGHT:
		return type - 2;
	case RIG_BACKGROUND:
		return type - 3;
	}
	return 0;
}

static int cfm_index_to_rig_type(int index)
{
	switch (index)
	{
	case 0:
	case 1:
	case 2:
		return index;
	case 3:
		return index + 1;
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
		return index + 2;
	case 12:
		return index + 3;
	}
	return 0;
}

static INT_PTR cfm_combo_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
	{
		struct modal_open_struct* mos = (struct modal_open_struct*)lparam;
		SetWindowTextW(hwnd, mos->title);
		WCHAR* curr = mos->choice_array;
		for (int i = 0; i < mos->choice_length; i++)
		{
			ComboBox_AddString(GetDlgItem(hwnd, IDCOMBO), curr);
			curr += wcsnlen(curr, 32);
			RUNTIME_ASSERT(*curr == '\0');
			curr++;
		}
		ComboBox_SetCurSel(GetDlgItem(hwnd, IDCOMBO), mos->choice_first);
		SetWindowLongPtrW(hwnd, DWLP_USER, (LONG_PTR)mos);
		break;
	}
	case WM_COMMAND:
		if (HIWORD(wparam) != BN_CLICKED)
		{
			return 0;
		}
		int index = ComboBox_GetCurSel(GetDlgItem(hwnd, IDCOMBO));
		if (index == -1)
		{
			EndDialog(hwnd, 1);
			return 0;
		}
		if (LOWORD(wparam) == IDOK)
		{
			struct modal_open_struct* mos = (struct modal_open_struct*)GetWindowLongPtrW(hwnd, DWLP_USER);
			RUNTIME_ASSERT(ComboBox_GetLBTextLen(GetDlgItem(hwnd, IDCOMBO), index) < sizeof mos->result / sizeof * mos->result);
			ComboBox_GetLBText(GetDlgItem(hwnd, IDCOMBO), index, mos->result);
			EndDialog(hwnd, 0);
		}
		else if (LOWORD(wparam) == IDCANCEL)
		{
			struct modal_open_struct* mos = (struct modal_open_struct*)GetWindowLongPtrW(hwnd, DWLP_USER);
			RUNTIME_ASSERT(ComboBox_GetLBTextLen(GetDlgItem(hwnd, IDCOMBO), index) < sizeof mos->result / sizeof * mos->result);
			ComboBox_GetLBText(GetDlgItem(hwnd, IDCOMBO), index, mos->result);
			EndDialog(hwnd, 1);
		}
		break;
	}
	return 0;
}

static void cfm_enum_internal(HWND owner, const WCHAR* title, int* value, WCHAR* array, int len, conversion_func type_to_index, conversion_func index_to_type)
{
	struct modal_open_struct mos = { .title = title };
	mos.choice_array = array;
	mos.choice_length = len;
	mos.choice_first = type_to_index ? type_to_index(*value) : *value;
	if (mos.choice_first < 0 && mos.choice_first >= mos.choice_length)
	{
		mos.choice_first = 0;
	}

	if (DialogBoxParamW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDD_CHANGE_FIELD_DROPDOWN), owner, cfm_combo_proc, (LPARAM)&mos) == 1)
	{
		return;
	}

	for (int i = 0; i < mos.choice_length; i++)
	{
		if (wcsncmp(mos.result, mos.choice_array, 32) == 0)
		{
			*value = index_to_type ? index_to_type(i) : i;
			return;
		}
		mos.choice_array += wcsnlen(mos.choice_array, 32) + 1;
	}
}

void change_field_modal_mineral_size(HWND owner, dnr_mineral_size_t* value)
{
#define ADD_SERIALIZABLE_ENUM(name, value) #name L"\0"
	WCHAR* array = SERIALIZABLE_DNR_MINERAL_SIZE;
#undef ADD_SERIALIZABLE_ENUM
	int res = (int)(*value - MINERAL_SIZE_5);
	cfm_enum_internal(owner, L"Change mineral size", &res, array, 6, NULL, NULL);
	*value = res + MINERAL_SIZE_5;
}

void change_field_modal_mineral_type(HWND owner, dnr_mineral_type_t* value)
{
#define ADD_SERIALIZABLE_ENUM(name, value) #name L"\0"
	WCHAR* array = SERIALIZABLE_DNR_MINERAL_TYPE;
#undef ADD_SERIALIZABLE_ENUM
	/* dnr_mineral_type is not a 32-bit integer */
	int _value = *value;
	cfm_enum_internal(owner, L"Change mineral type", &_value, array, 8, cfm_mineral_type_to_index, cfm_index_to_mineral_type);
	*value = _value;
}

void change_field_modal_rig_type(HWND owner, dnr_rig_type_t* value)
{
#define ADD_SERIALIZABLE_ENUM(name, value) #name L"\0"
	WCHAR* array = SERIALIZABLE_DNR_RIG_TYPE;
#undef ADD_SERIALIZABLE_ENUM
	cfm_enum_internal(owner, L"Change rig type", (int*)value, array, 13, cfm_rig_type_to_index, cfm_index_to_rig_type);
}

void change_field_modal_mineral_move_direction(HWND owner, dnr_mineral_move_direction_t* value)
{
#define ADD_SERIALIZABLE_ENUM(name, value) #name L"\0"
	WCHAR* array = SERIALIZABLE_DNR_MINERAL_MOVE_DIRECTION;
#undef ADD_SERIALIZABLE_ENUM
	int res = (int)(*value);
	if (res == MOVE_DIRECTION_DOWN)
	{
		res--;
	}
	cfm_enum_internal(owner, L"Change mineral move direction", &res, array, 4, NULL, NULL);
	if (res == 3)
	{
		res++;
	}
	*value = (dnr_mineral_move_direction_t)res;
}

void change_field_modal_mineral_spawn_rule(HWND owner, dnr_mineral_spawn_rule_t* value)
{
#define ADD_SERIALIZABLE_ENUM(name, value) #name L"\0"
	WCHAR* array = SERIALIZABLE_DNR_MINERAL_SPAWN_RULE;
#undef ADD_SERIALIZABLE_ENUM
	cfm_enum_internal(owner, L"Change mineral spawn rule", (int*)value, array, 10, NULL, NULL);
}

void change_field_modal_weather_type(HWND owner, dnr_weather_type_t* value)
{
#define ADD_SERIALIZABLE_ENUM(name, value) #name L"\0"
	WCHAR* array = SERIALIZABLE_DNR_WEATHER_TYPE;
#undef ADD_SERIALIZABLE_ENUM
	cfm_enum_internal(owner, L"Change weather type", (int*)value, array, 4, NULL, NULL);
}

void change_field_modal_asset_tile_type(HWND owner, asset_tile_type_t* value)
{
#define ADD_SERIALIZABLE_ENUM(name, value) #name L"\0"
	WCHAR* array = SERIALIZABLE_ASSET_TILE_TYPE;
#undef ADD_SERIALIZABLE_ENUM
	cfm_enum_internal(owner, L"Change asset tile type", (int*)value, array, 16, NULL, NULL);
}

static INT_PTR cfm_text_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
	{
		struct modal_open_struct* mos = (struct modal_open_struct*)lparam;
		SetWindowTextW(hwnd, mos->title);
		Edit_SetText(GetDlgItem(hwnd, IDEDIT), mos->choice_array);
		SetWindowLongPtrW(hwnd, DWLP_USER, (LONG_PTR)mos);
		break;
	}
	case WM_COMMAND:
		if (HIWORD(wparam) != BN_CLICKED)
		{
			return 0;
		}
		if (LOWORD(wparam) == IDOK)
		{
			struct modal_open_struct* mos = (struct modal_open_struct*)GetWindowLongPtrW(hwnd, DWLP_USER);
			Edit_GetText(GetDlgItem(hwnd, IDEDIT), mos->result, sizeof mos->result / sizeof * mos->result);
			EndDialog(hwnd, 0);
		}
		else if (LOWORD(wparam) == IDCANCEL)
		{
			struct modal_open_struct* mos = (struct modal_open_struct*)GetWindowLongPtrW(hwnd, DWLP_USER);
			Edit_GetText(GetDlgItem(hwnd, IDEDIT), mos->result, sizeof mos->result / sizeof * mos->result);
			EndDialog(hwnd, 1);
		}
		break;
	}
	return 0;
}

void change_field_modal_integer(HWND owner, void* value, int bitmask_size)
{
	bool is_signed = bitmask_size & SIZE_IS_SIGNED;
	bitmask_size = bitmask_size & ~SIZE_IS_SIGNED;

	WCHAR buf[64];
	struct modal_open_struct mos = { .title = L"Change unsigned integer type", .choice_array = buf };
	if (is_signed)
	{
		mos.title = L"Change signed integer type";
	}
	switch (bitmask_size)
	{
	case sizeof(uint8_t):
		StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%#004x", *(uint8_t*)value);
		break;
	case sizeof(uint16_t):
		StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%#006x", *(uint16_t*)value);
		break;
	case sizeof(uint32_t):
		if (is_signed)
		{
			StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%i", *(int32_t*)value);
		}
		else
		{
			StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%#010x", *(uint32_t*)value);
		}
		break;
	}
	if (DialogBoxParamW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDD_CHANGE_FIELD_TEXT), owner, cfm_text_proc, (LPARAM)&mos) == 1)
	{
		return;
	}
	int result = 0;
	if (is_signed)
	{
		result = wcstol(mos.result, NULL, 0);
	}
	else
	{
		result = wcstoul(mos.result, NULL, 0);
	}
	switch (bitmask_size)
	{
	case sizeof(uint8_t):
		*(uint8_t*)value = result;
		break;
	case sizeof(uint16_t):
		*(uint16_t*)value = result;
		break;
	case sizeof(uint32_t):
		if (is_signed)
		{
			*(int32_t*)value = result;
		}
		else
		{
			*(uint32_t*)value = result;
		}
		break;
	}
}

void change_field_modal_float(HWND owner, float* value)
{
	WCHAR buf[64];
	struct modal_open_struct mos = { .title = L"Change float type", .choice_array = buf };
	StringCchPrintfW(buf, sizeof buf / sizeof * buf, L"%f", *value);
	DialogBoxParamW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDD_CHANGE_FIELD_TEXT), owner, cfm_text_proc, (LPARAM)&mos);
	*value = (float)wcstod(mos.result, NULL);
}

static void cfm_populate_combobox(HWND hwnd, int length, const WCHAR* str)
{
	for (int i = 0; i < length; i++)
	{
		ComboBox_AddString(hwnd, str);
		str += wcsnlen(str, 32);
		RUNTIME_ASSERT(*str == '\0');
		str++;
	}
}

static void cfm_invalidate_charinfo(HWND hwnd)
{
	struct modal_charinfo_open_struct* mos = (struct modal_charinfo_open_struct*)GetWindowLongPtrW(hwnd, DWLP_USER);
	MINERAL_CONTROL_SET_CELL(GetDlgItem(hwnd, IDMINERAL), mos->curr_value->Char.AsciiChar, mos->curr_value->Attributes, DNR_DEFAULT_DIRT_COLOR);

	char buf[256];
	snprintf(buf, sizeof buf, "Character:%#04x\nAttribute:%#06x", mos->curr_value->Char.AsciiChar & 0xFF, mos->curr_value->Attributes & 0xFFFF);
	SetWindowTextA(GetDlgItem(hwnd, IDSELECTEDDESCRIPTION), buf);
}

static INT_PTR cfm_charinfo_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
	{
		struct modal_charinfo_open_struct* mos = (struct modal_charinfo_open_struct*)lparam;
		SetWindowTextW(hwnd, mos->title);

		cfm_populate_combobox(GetDlgItem(hwnd, IDFOREGROUND), 16, L"DARK_BLACK\0DARK_BLUE\0DARK_GREEN\0DARK_AQUA\0DARK_RED\0DARK_PURPLE\0DARK_YELLOW\0LIGHT_GRAY\0DARK_GRAY\0LIGHT_BLUE\0LIGHT_GREEN\0LIGHT_AQUA\0LIGHT_RED\0LIGHT_PURPLE\0LIGHT_YELLOW\0LIGHT_WHITE");
		cfm_populate_combobox(GetDlgItem(hwnd, IDBACKGROUND), 16, L"DARK_BLACK\0DARK_BLUE\0DARK_GREEN\0DARK_AQUA\0DARK_RED\0DARK_PURPLE\0DARK_YELLOW\0LIGHT_GRAY\0DARK_GRAY\0LIGHT_BLUE\0LIGHT_GREEN\0LIGHT_AQUA\0LIGHT_RED\0LIGHT_PURPLE\0LIGHT_YELLOW\0LIGHT_WHITE");
		
		ComboBox_SetCurSel(GetDlgItem(hwnd, IDFOREGROUND), ATTRIBUTE_FOREGROUND(mos->curr_value->Attributes));
		ComboBox_SetCurSel(GetDlgItem(hwnd, IDBACKGROUND), ATTRIBUTE_BACKGROUND(mos->curr_value->Attributes));

		SetWindowLongPtrW(hwnd, DWLP_USER, (LONG_PTR)mos);
		cfm_invalidate_charinfo(hwnd);
		break;
	}
	case WM_COMMAND:
		if (HIWORD(wparam) == BN_CLICKED)
		{
			if (LOWORD(wparam) == IDOK)
			{
				struct modal_charinfo_open_struct* mos = (struct modal_charinfo_open_struct*)GetWindowLongPtrW(hwnd, DWLP_USER);
				*mos->value = *mos->curr_value;
				EndDialog(hwnd, 0);
			}
			else if (LOWORD(wparam) == IDCANCEL)
			{
				EndDialog(hwnd, 1);
			}
		}
		else if (HIWORD(wparam) == CBN_SELCHANGE)
		{
			struct modal_charinfo_open_struct* mos = (struct modal_charinfo_open_struct*)GetWindowLongPtrW(hwnd, DWLP_USER);
			if (LOWORD(wparam) == IDFOREGROUND)
			{
				mos->curr_value->Attributes = mos->curr_value->Attributes & 0xF0 | ComboBox_GetCurSel(GetDlgItem(hwnd, IDFOREGROUND));
			}
			else if (LOWORD(wparam) == IDBACKGROUND)
			{
				mos->curr_value->Attributes = mos->curr_value->Attributes & 0x0F | ((ComboBox_GetCurSel(GetDlgItem(hwnd, IDBACKGROUND)) & 0x0F) << 4);
			}
			cfm_invalidate_charinfo(hwnd);
		}
		else if (HIWORD(wparam) == CHARMAP_CONTROL_CURRENT_SET)
		{
			struct modal_charinfo_open_struct* mos = (struct modal_charinfo_open_struct*)GetWindowLongPtrW(hwnd, DWLP_USER);
			mos->curr_value->Char.AsciiChar = (char)CHARMAP_CONTROL_GET_CURRENT(GetDlgItem(hwnd, IDCHARACTERPICKER));
			cfm_invalidate_charinfo(hwnd);
		}
		break;
	}
	return 0;
}

void change_field_modal_char_info(HWND owner, CHAR_INFO* value)
{
	CHAR_INFO copy = *value;
	struct modal_charinfo_open_struct mos = { .title = L"Change char info type", .curr_value = &copy, .value = value };
	DialogBoxParamW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDD_CHANGE_FIELD_CHARINFO), owner, cfm_charinfo_proc, (LPARAM)&mos);
}

void change_field_modal_color(HWND owner, color_t* value)
{
	CHOOSECOLOR cc = { 0 };
	static COLORREF cust[16] = { 0 };

	cc.lStructSize = sizeof cc;
	cc.hwndOwner = owner;
	cc.hInstance = NULL;
	cc.lpCustColors = cust;
	cc.rgbResult = (COLORREF)(*value);
	cc.Flags = CC_FULLOPEN | CC_RGBINIT;

	if (ChooseColorW(&cc))
	{
		*value = (color_t)cc.rgbResult;
	}
}