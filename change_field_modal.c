/*
	change_field_modal.c ~ RL
*/

#include "change_field_modal.h"
#include "file.h"
#include "resource.h"
#include <windowsx.h>

struct modal_open_struct
{
	const WCHAR* title;
	int choice_first, choice_length;
	WCHAR* choice_array;
	WCHAR result[32];
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

static INT_PTR cfm_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
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
		if (LOWORD(wparam) == IDOK)
		{
			struct modal_open_struct* mos = (struct modal_open_struct*)GetWindowLongPtrW(hwnd, DWLP_USER);
			int index = ComboBox_GetCurSel(GetDlgItem(hwnd, IDCOMBO));
			RUNTIME_ASSERT(ComboBox_GetLBTextLen(GetDlgItem(hwnd, IDCOMBO), index) < sizeof mos->result / sizeof * mos->result);
			ComboBox_GetLBText(GetDlgItem(hwnd, IDCOMBO), index, mos->result);
			EndDialog(hwnd, 0);
		}
		else if (LOWORD(wparam) == IDCANCEL)
		{
			EndDialog(hwnd, 1);
		}
		break;
	}
	return 0;
}

static void cfm_enum_internal(HWND owner, int* value, WCHAR* array, int len, conversion_func type_to_index, conversion_func index_to_type)
{
	struct modal_open_struct mos = { .title = L"Change mineral type" };
	mos.choice_array = array;
	mos.choice_length = len;
	mos.choice_first = type_to_index ? type_to_index(*value) : *value;
	RUNTIME_ASSERT(mos.choice_first >= 0 && mos.choice_first < mos.choice_length);

	DialogBoxParamW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDD_CHANGE_FIELD_DROPDOWN), owner, cfm_proc, (LPARAM)&mos);

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
	cfm_enum_internal(owner, &res, array, 6, NULL, NULL);
	*value = res + MINERAL_SIZE_5;
}

void change_field_modal_mineral_type(HWND owner, dnr_mineral_type_t* value)
{
#define ADD_SERIALIZABLE_ENUM(name, value) #name L"\0"
	WCHAR* array = SERIALIZABLE_DNR_MINERAL_TYPE;
#undef ADD_SERIALIZABLE_ENUM
	cfm_enum_internal(owner, (int*)value, array, 8, cfm_mineral_type_to_index, cfm_index_to_mineral_type);
}

void change_field_modal_rig_type(HWND owner, dnr_rig_type_t* value)
{
#define ADD_SERIALIZABLE_ENUM(name, value) #name L"\0"
	WCHAR* array = SERIALIZABLE_DNR_RIG_TYPE;
#undef ADD_SERIALIZABLE_ENUM
	cfm_enum_internal(owner, (int*)value, array, 13, cfm_rig_type_to_index, cfm_index_to_rig_type);
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
	cfm_enum_internal(owner, &res, array, 4, NULL, NULL);
	if (res == MOVE_DIRECTION_DOWN)
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
	cfm_enum_internal(owner, (int*)value, array, 10, NULL, NULL);
}