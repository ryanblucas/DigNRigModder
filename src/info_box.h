/*
	info_box.h ~ RL
	Window that increases interactivity with the main display
*/

#pragma once

#include "file.h"
#include "game.h"
#include "serialize.h"

#define INFO_BRUSH_MIN_SIZE 1
#define INFO_BRUSH_MAX_SIZE 6
#define INFO_BOX_CLIENT_WIDTH 450
#define INFO_BOX_CLIENT_HEIGHT 392

typedef enum info_tool
{
	TOOL_ERASER,
	TOOL_SELECT,
	TOOL_BRUSH,
} info_tool_t;

typedef void (*info_handle_change_mode_t)(info_mode_t old_mode);

typedef void (*info_handle_change_tool_t)(info_tool_t new_tool);
typedef void (*info_handle_change_brush_size_t)(int size);
typedef void (*info_handle_change_brush_block_t)(const complete_block_t* brush);
typedef void (*info_handle_change_block_t)(region_t region);
typedef void (*info_handle_change_global_field_t)(const void* field);

typedef void (*info_handle_change_file_t)(const char* directory);

typedef struct info_events
{
	info_handle_change_tool_t tool_handler;
	info_handle_change_brush_size_t brush_size_handler;
	info_handle_change_brush_block_t brush_block_handler;
	info_handle_change_block_t block_handler;
	info_handle_change_global_field_t global_field_handler;

	info_handle_change_file_t file_handler;
} info_events_t;

typedef struct info_internal
{
	HFONT font_caption, font_text;
	HWND window;
	const info_events_t* events;
} info_internal_t;

typedef bool (*info_mode_proc_t)(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, LRESULT* out);
typedef void (*info_mode_show_t)(bool is_visible);
typedef void (*info_mode_initialize_t)(const info_internal_t* internal);
typedef void (*info_mode_destroy_t)();

typedef struct info_mode_class
{
	info_mode_t mode;
	const char* caption;
	info_mode_proc_t proc;
	info_mode_show_t show;
	info_mode_initialize_t initialize;
	info_mode_destroy_t destroy;
} info_mode_class_t;

void info_add_class(const info_mode_class_t* class);
void info_set_event_handlers(const info_events_t* events);
void info_initialize(info_handle_change_mode_t change_mode);
void info_destroy(void);

info_mode_t info_get_current_mode(void);
void info_set_current_mode(info_mode_t mode);