/*
	change_field_modal.h ~ RL
*/

#pragma once

#include "serialize.h"
#include "types.h"

void change_field_modal_mineral_size(HWND owner, dnr_mineral_size_t* value);
void change_field_modal_mineral_type(HWND owner, dnr_mineral_type_t* value);
void change_field_modal_rig_type(HWND owner, dnr_rig_type_t* value);
void change_field_modal_mineral_move_direction(HWND owner, dnr_mineral_move_direction_t* value);
void change_field_modal_mineral_spawn_rule(HWND owner, dnr_mineral_spawn_rule_t* value);

#define SIZE_IS_SIGNED 0x80000000

void change_field_modal_integer(HWND owner, void* value, int bitmask_size);
void change_field_modal_float(HWND owner, float* value);

void change_field_modal_char_info(HWND owner, CHAR_INFO* value);

void change_field_modal_color(HWND owner, color_t* value);