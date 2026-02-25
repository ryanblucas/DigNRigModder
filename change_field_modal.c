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

void change_field_modal_mineral_size(HWND owner, dnr_mineral_size_t* value)
{
	struct modal_open_struct mos = { .title = L"Change mineral size" };
#define ADD_SERIALIZABLE_ENUM(name, value) #name L"\0"
	mos.choice_array = SERIALIZABLE_DNR_MINERAL_SIZE;
#undef ADD_SERIALIZABLE_ENUM
	mos.choice_length = 6;
	mos.choice_first = *value - MINERAL_SIZE_5;
	RUNTIME_ASSERT(mos.choice_first >= 0 && mos.choice_first < mos.choice_length);

	DialogBoxParamW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDD_CHANGE_FIELD_DROPDOWN), owner, cfm_proc, (LPARAM)&mos);

	for (int i = 0; i < mos.choice_length; i++)
	{
		if (wcsncmp(mos.result, mos.choice_array, 32) == 0)
		{
			*value = (dnr_mineral_size_t)(i + MINERAL_SIZE_5);
			return;
		}
		mos.choice_array += wcsnlen(mos.choice_array, 32) + 1;
	}
}

static inline int cfm_mineral_type_to_index(dnr_mineral_type_t type)
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

static inline dnr_mineral_type_t cfm_index_to_mineral_type(int index)
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

void change_field_modal_mineral_type(HWND owner, dnr_mineral_type_t* value)
{
	struct modal_open_struct mos = { .title = L"Change mineral type" };
#define ADD_SERIALIZABLE_ENUM(name, value) #name L"\0"
	mos.choice_array = SERIALIZABLE_DNR_MINERAL_TYPE;
#undef ADD_SERIALIZABLE_ENUM
	mos.choice_length = 8;
	mos.choice_first = cfm_mineral_type_to_index(*value);
	RUNTIME_ASSERT(mos.choice_first >= 0 && mos.choice_first < mos.choice_length);

	DialogBoxParamW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDD_CHANGE_FIELD_DROPDOWN), owner, cfm_proc, (LPARAM)&mos);

	for (int i = 0; i < mos.choice_length; i++)
	{
		if (wcsncmp(mos.result, mos.choice_array, 32) == 0)
		{
			*value = cfm_index_to_mineral_type(i);
			return;
		}
		mos.choice_array += wcsnlen(mos.choice_array, 32) + 1;
	}
}