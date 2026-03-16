/*
	charmap_control.h ~ RL
	Creates a control for displaying a character map
*/

#pragma once

#include <Windows.h>

#define CHARMAP_CONTROL_CLASS_NAME L"dnr_charmap_control"

#define CHARMAP_CONTROL_SET_CURRENT(hwnd, character) SendMessageW(hwnd, CHARMAP_CONTROL_MSG_SET_CURRENT, (WPARAM)(character), 0)
#define CHARMAP_CONTROL_MSG_SET_CURRENT (WM_USER + 0x20)
#define CHARMAP_CONTROL_GET_CURRENT(hwnd) SendMessageW(hwnd, CHARMAP_CONTROL_MSG_GET_CURRENT, 0, 0)
#define CHARMAP_CONTROL_MSG_GET_CURRENT (WM_USER + 0x21)

#define CHARMAP_CONTROL_CURRENT_SET 2

void charmap_control_initialize(void);
void charmap_control_destroy(void);