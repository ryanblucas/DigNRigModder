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

typedef enum info_mode
{
	MODE_SAVE,
	//MODE_SPRITE,
	MODE_LAYER,

	MODE_COUNT
} info_mode_t;

typedef enum info_tool
{
	TOOL_ERASER,
	TOOL_SELECT,
	TOOL_BRUSH,
} info_tool_t;

typedef void (*info_handle_change_mode)(info_mode_t new_mode);

typedef void (*info_handle_change_tool)(info_tool_t new_tool);
typedef void (*info_handle_change_brush_size)(int size);
typedef void (*info_handle_change_brush_block)(const complete_block_t* brush);
typedef void (*info_handle_change_block)(region_t region);
typedef void (*info_handle_change_global_field)(const void* field);

typedef void (*info_handle_change_file)(const char* directory);

typedef struct info_events
{
	info_handle_change_tool tool_handler;
	info_handle_change_brush_size brush_size_handler;
	info_handle_change_brush_block brush_block_handler;
	info_handle_change_block block_handler;
	info_handle_change_global_field global_field_handler;

	info_handle_change_file file_handler;
} info_events_t;

typedef struct info_internal
{
	HFONT font_caption, font_text;
	HWND window;
	const info_events_t* events;
} info_internal_t;

typedef bool (*info_mode_proc)(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, LRESULT* out);
typedef void (*info_mode_show)(bool is_visible);
typedef void (*info_mode_initialize)(const info_internal_t* internal);
typedef void (*info_mode_destroy)();

typedef struct info_mode_class
{
	info_mode_t mode;
	const char* caption;
	info_mode_proc proc;
	info_mode_show show;
	info_mode_initialize initialize;
	info_mode_destroy destroy;
} info_mode_class_t;

void info_add_class(const info_mode_class_t* class);
void info_set_event_handlers(const info_events_t* events);
void info_initialize(info_handle_change_mode change_mode);
void info_destroy(void);

info_mode_t info_get_current_mode(void);