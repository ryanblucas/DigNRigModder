/*
	mineral_palette.h ~ RL
	Implements a matrix of asset_block_t's for the user to select from when using the brush tool.
*/

#pragma once

#include "../file.h"

#define MINERAL_PALETTE_CLASS_NAME L"dnr_mineral_palette"

#define MINERAL_PALETTE_MSG_SET_CELL_SIZE (WM_USER + 0x30)
#define MINERAL_PALETTE_MSG_SET_CELL (WM_USER + 0x31)
#define MINERAL_PALETTE_SET_CELL_SIZE(hwnd, multiplier) SendMessageW(hwnd, MINERAL_PALETTE_MSG_SET_CELL_SIZE, (WPARAM)(multiplier), 0)
#define MINERAL_PALETTE_SET_CELL(hwnd, index, block) SendMessageW(hwnd, MINERAL_PALETTE_MSG_SET_CELL, (WPARAM)(index), (LPARAM)((const asset_block_t*)(block)))

#define MINERAL_PALETTE_MSG_GET_INDEX_FROM_POSITION (WM_USER + 0x32)
#define MINERAL_PALETTE_MSG_GET_CELL (WM_USER + 0x33)
#define MINERAL_PALETTE_GET_INDEX_FROM_POSITION(hwnd, x, y) SendMessageW(hwnd, MINERAL_PALETTE_MSG_GET_INDEX_FROM_POSITION, (WPARAM)MAKEWPARAM(x, y), 0)
#define MINERAL_PALETTE_GET_CELL(hwnd, i) SendMessageW(hwnd, MINERAL_PALETTE_MSG_GET_CELL, (WPARAM)i, 0)

#define MINERAL_PALETTE_MSG_SELECT_CELL (WM_USER + 0x34)
#define MINERAL_PALETTE_MSG_GET_SELECTED_CELL (WM_USER + 0x35)
#define MINERAL_PALETTE_SELECT_CELL(hwnd, index) SendMessageW(hwnd, MINERAL_PALETTE_MSG_SELECT_CELL, (WPARAM)(index), 0)
#define MINERAL_PALETTE_GET_SELECTED_CELL(hwnd) SendMessageW(hwnd, MINERAL_PALETTE_MSG_GET_SELECTED_CELL, 0, 0)

void mineral_palette_initialize(void);
void mineral_palette_destroy(void);