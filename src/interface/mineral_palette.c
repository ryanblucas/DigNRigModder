/*
	mineral_palette.c ~ RL
	Implements a matrix of asset_block_t's for the user to select from when using the brush tool.
*/

#include "mineral_palette.h"
#include "mineral_control.h"
#include "../debug.h"
#include <math.h>
#include <Windows.h>

#define START_CELL_SIZE 8

struct mineral_palette_internal
{
	int cell_size;
	int width, height;
	int selected_index;
	size_t blocks_reserved;
	asset_block_t blocks[];
};

static bool already_initialized;

static LRESULT mineral_palette_window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	struct mineral_palette_internal* mpi = (struct mineral_palette_internal*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	switch (msg)
	{
	case WM_CREATE:
	{
		CREATESTRUCTW* cs = (CREATESTRUCTW*)lparam;
		size_t reserved = (size_t)(cs->cx / START_CELL_SIZE) * (size_t)(cs->cy / START_CELL_SIZE);
		reserved = (size_t)1 << (size_t)(log2(reserved) + 1);

		struct mineral_palette_internal* mpi = dig_malloc(sizeof * mpi + reserved * sizeof * mpi->blocks);
		memset(mpi, 0, sizeof * mpi + reserved * sizeof * mpi->blocks);
		mpi->blocks_reserved = reserved;
		mpi->cell_size = START_CELL_SIZE;
		mpi->width = cs->cx / mpi->cell_size;
		mpi->height = cs->cy / mpi->cell_size;
		mpi->selected_index = -1;
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)mpi);

		RECT rect;
		RUNTIME_ASSERT(GetWindowRect(hwnd, &rect));
		rect.right = rect.left + mpi->width * mpi->cell_size;
		rect.bottom = rect.top + mpi->height * mpi->cell_size;
		RUNTIME_ASSERT(SetWindowPos(hwnd, NULL, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOMOVE));

		break;
	}
	case WM_SIZING:
	{
		RECT* new_size = (RECT*)lparam;
		int width = new_size->right - new_size->left;
		int height = new_size->bottom - new_size->top;
		size_t reserved = (size_t)width * (size_t)height / (mpi->cell_size * mpi->cell_size);
		reserved = (size_t)1 << (size_t)(log2(reserved) + 1);
		if (reserved > mpi->blocks_reserved)
		{
			struct mineral_palette_internal* new_mpi = dig_malloc(sizeof * new_mpi + reserved * sizeof * new_mpi->blocks);
			memset(new_mpi, 0, sizeof * new_mpi + reserved * sizeof * new_mpi->blocks);
			new_mpi->blocks_reserved = reserved;
			new_mpi->cell_size = mpi->cell_size;
			memcpy(new_mpi->blocks, mpi->blocks, mpi->blocks_reserved * sizeof * mpi->blocks);
			free(mpi);
			mpi = new_mpi;
			SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)mpi);
		}
		mpi->width = width / mpi->cell_size;
		mpi->height = height / mpi->cell_size;
		new_size->right = new_size->left + mpi->width * mpi->cell_size;
		new_size->bottom = new_size->top + mpi->height * mpi->cell_size;

		if (mpi->selected_index >= mpi->width * mpi->height)
		{
			mpi->selected_index = -1;
		}

		InvalidateRect(hwnd, NULL, TRUE);

		break;
	}
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		RUNTIME_ASSERT(BeginPaint(hwnd, &ps));
		for (int i = 0; i < mpi->height; i++)
		{
			for (int j = 0; j < mpi->width; j++)
			{
				CHAR_INFO curr = mpi->blocks[j + i * mpi->width].visual;
				if (j + i * mpi->width == mpi->selected_index)
				{
					curr.Attributes = ~curr.Attributes & 0xFF;
				}
				mineral_control_gdi_render_cell(ps.hdc, j * mpi->cell_size, i * mpi->cell_size, mpi->cell_size, mpi->cell_size, curr, DNR_DEFAULT_DIRT_COLOR);
			}
		} 
		EndPaint(hwnd, &ps);
		break;
	}
	case WM_LBUTTONDOWN:
	{
		MINERAL_PALETTE_SELECT_CELL(hwnd, MINERAL_PALETTE_GET_INDEX_FROM_POSITION(hwnd, LOWORD(lparam), HIWORD(lparam)));
		break;
	}
	case MINERAL_PALETTE_MSG_SET_CELL_SIZE:
	{
		mpi->cell_size = (int)wparam * TARGET_CELL_SIZE;

		RECT new_size;
		RUNTIME_ASSERT(GetWindowRect(hwnd, &new_size));
		mpi->width = (new_size.right - new_size.left) / mpi->cell_size;
		mpi->height = (new_size.bottom - new_size.top) / mpi->cell_size;
		new_size.right = new_size.left + mpi->width * mpi->cell_size;
		new_size.bottom = new_size.top + mpi->height * mpi->cell_size;
		RUNTIME_ASSERT(SetWindowPos(hwnd, NULL, 0, 0, new_size.right - new_size.left, new_size.bottom - new_size.top, SWP_NOMOVE));
		InvalidateRect(hwnd, NULL, TRUE);
		break;
	}
	case MINERAL_PALETTE_MSG_SET_CELL:
	{
		int index = (int)wparam;
		const asset_block_t* block = (asset_block_t*)lparam;

		RUNTIME_ASSERT(index >= 0 && index < mpi->width * mpi->height);
		mpi->blocks[index] = *block;
		InvalidateRect(hwnd, NULL, TRUE);
		break;
	}
	case MINERAL_PALETTE_MSG_GET_INDEX_FROM_POSITION:
	{
		int x = LOWORD(wparam);
		int y = HIWORD(wparam);

		if (x < 0 || y < 0 || x >= mpi->width * mpi->cell_size || y >= mpi->height * mpi->cell_size)
		{
			return (LRESULT)-1;
		}

		x /= mpi->cell_size;
		y /= mpi->cell_size;

		return (LRESULT)(x + y * mpi->width);
	}
	case MINERAL_PALETTE_MSG_GET_CELL:
	{
		RUNTIME_ASSERT((int)wparam >= 0 && (int)wparam < mpi->width * mpi->height);
		return (LRESULT)(&mpi->blocks[(int)wparam]);
	}
	case MINERAL_PALETTE_MSG_SELECT_CELL:
	{
		mpi->selected_index = (int)wparam;
		RUNTIME_ASSERT(mpi->selected_index >= 0 && mpi->selected_index < mpi->width * mpi->height);
		InvalidateRect(hwnd, NULL, TRUE);
		break;
	}
	case MINERAL_PALETTE_MSG_GET_SELECTED_CELL:
	{
		return (LRESULT)mpi->selected_index;
	}
	case WM_DESTROY:
		free(mpi);
		break;
	default:
		return DefWindowProcW(hwnd, msg, wparam, lparam);
	}
	return 0;
}

void mineral_palette_initialize(void)
{
	if (already_initialized)
	{
		return;
	}

	WNDCLASSW class = { 0 };
	class.lpszClassName = MINERAL_PALETTE_CLASS_NAME;
	class.lpfnWndProc = mineral_palette_window_proc;
	RUNTIME_ASSERT(RegisterClassW(&class));
	already_initialized = true;
}

void mineral_palette_destroy(void)
{
	if (!already_initialized)
	{
		return;
	}

	UnregisterClassW(MINERAL_PALETTE_CLASS_NAME, NULL);
	already_initialized = false;
}