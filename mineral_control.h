/*
	mineral_control.h ~ RL
*/

#pragma once

#include "screen.h"

#define MINERAL_CONTROL_CLASS_NAME L"dnr_mineral_control"

#define MINERAL_CONTROL_MSG_SET_CELL (WM_USER + 0x10)
#define MINERAL_CONTROL_SET_CELL(hwnd, character, attrib, dirt_color) SendMessage(hwnd, MINERAL_CONTROL_MSG_SET_CELL, (WPARAM)PACK_CELL(character, attrib), (LPARAM)(dirt_color))
#define PACK_CELL(character, attrib) ((character) & 0xFF) | (((attrib) << 0x8) & 0xFFFF00)
#define PACK_CELL_CHARACTER(packed) ((packed) & 0xFF)
#define PACK_CELL_ATTRIBUTE(packed) (((packed) >> 8) & 0xFFFF)

void mineral_control_initialize(void);
void mineral_control_destroy(void);