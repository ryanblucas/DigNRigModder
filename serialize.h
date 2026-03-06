/*
	serialize.h ~ RL
*/

#pragma once

#include "file.h"
#include <Windows.h>
#include <commctrl.h>

typedef struct element* element_t;

/* by running serialize_hash on each of the types as a string, you get this result. */

#define TYPE_FLOAT 210624726069ULL
#define TYPE_RGB_COLOR_T 13790352177619491131ULL
#define TYPE_BOOLEAN32_T 13766221191973021547ULL
#define TYPE_CHAR_INFO 249834764690065676ULL
#define TYPE_INT32_T 229378475688636ULL
#define TYPE_UINT32_T 7569686425136137ULL
#define TYPE_DNR_POINTER_T 15207900801384124882ULL
#define TYPE_UINT16_T 7569686425208015ULL
#define TYPE_UINT8_T 229384437135984ULL
#define TYPE_DNR_SPRITE_T 11081697800589051136ULL
#define TYPE_DNR_RIG_TYPE_T 3798355938377845426ULL
#define TYPE_DNR_MINERAL_MOVE_DIRECTION_T 7836251078612970605ULL
#define TYPE_DNR_MINERAL_SPAWN_RULE_T 567133761540061260ULL
#define TYPE_DNR_SAVE_HEADER_T 12164599792533459048ULL
#define TYPE_DNR_LAYER_HEADER_T 663611519707857130ULL
#define TYPE_DNR_PLAYER_T 11081698126627463866ULL
#define TYPE_DNR_BLOCK_T 13751622877662047520ULL
#define TYPE_DNR_MINERAL_SIZE_T 12303576239702824387ULL
#define TYPE_DNR_MINERAL_TYPE_T 12303576239558253438ULL
#define TYPE_DNR_MINERAL_T 15207893016492402041ULL

size_t serialize_element_get_size(const element_t element);
void serialize_element_get_name(const element_t element, char* buf, size_t buf_size);
uint64_t serialize_element_get_type(const element_t element);
const void* serialize_element_get_value(const element_t element);
int serialize_element_get_count(const element_t element);
HTREEITEM serialize_element_get_handle(const element_t element);

void serialize_element_set_value(element_t element, const void* value);

element_t serialize_element_get_parent(element_t element);
int serialize_element_get_index(const element_t element);
element_t serialize_element_get_from_node(HWND window, HTREEITEM item);

void serialize_element_delete(element_t element);

void serialize_single(const char* type, void* value, const char* name, HWND tree_window, HTREEITEM tree_item);
void serialize_array(const char* type, void* value, int count, const char* name, HWND tree_window, HTREEITEM tree_item);

void serialize_delete(HWND tree_window);

void serialize_on_expand(element_t element);
bool serialize_on_change_field(element_t element);

HTREEITEM serialize_tree_find_item(HWND tree_window, HTREEITEM root, const char* name);