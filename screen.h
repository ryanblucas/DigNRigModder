/*
	screen.h ~ RL
	Handles console screen
*/

#pragma once

#include "types.h"
#include <Windows.h>

#define CTRL_RIGHT_ALT_PRESSED		RIGHT_ALT_PRESSED
#define CTRL_LEFT_ALT_PRESSED		LEFT_ALT_PRESSED
#define CTRL_RIGHT_PRESSED			RIGHT_CTRL_PRESSED
#define CTRL_LEFT_PRESSED			LEFT_CTRL_PRESSED
#define CTRL_SHIFT_PRESSED			SHIFT_PRESSED

typedef uint32_t keyboard_control_t;
typedef uint16_t virtual_key_t;

typedef void (*screen_handle_repaint_t)();
typedef void (*screen_handle_key_t)(virtual_key_t key, keyboard_control_t ctrl);
typedef void (*screen_handle_mouse_button_t)(bool m1_down, int x, int y);
typedef void (*screen_handle_mouse_move_t)(bool m1_down, int x, int y);
typedef void (*screen_handle_mouse_wheel_t)(int delta);

typedef struct screen_events
{
	screen_handle_repaint_t repaint;
	screen_handle_key_t keyboard;
	screen_handle_mouse_button_t mouse_button;
	screen_handle_mouse_move_t mouse_move;
	screen_handle_mouse_wheel_t mouse_wheel;
} screen_events_t;

void screen_initialize(screen_events_t events);
void screen_destroy(void);
void screen_loop(void);
void screen_repaint(void);
void screen_invalidate(void);
void screen_clear(void);

rgb_color_t screen_dirt_color(void);

void screen_change_title(const char* title);
void screen_change_dirt_color(rgb_color_t rgb);

void screen_set_char_region(const char* in, region_t region);
void screen_set_attrib_region(const attribute_t* in, region_t region);

/* out must be wx * wy bytes long. Returns how many bytes were written from screen data. 
	If there are cells outside of the region provided, the result will still be in wx*wy dimensions. */
int screen_get_char_region(char* out, region_t region);
/* out must be wx * wy bytes long. Returns how many bytes were written from screen data.
	If there are cells outside of the region provided, the result will still be in wx*wy dimensions. */
int screen_get_attrib_region(attribute_t* out, region_t region);

sprite_t screen_sprite_create(int width, int height, rgb_color_t dirt_color, char* text, attribute_t* attrib);
void screen_sprite_destroy(sprite_t sprite);
void screen_sprite_render(int x, int y, const sprite_t sprite);

void screen_sprite_set_char_region(sprite_t sprite, const char* in, region_t region);
void screen_sprite_set_attrib_region(sprite_t sprite, const attribute_t* in, region_t region);

/* out must be wx * wy bytes long. Returns how many bytes were written from sprite data.
	If there are cells outside of the region provided, the result will still be in wx*wy dimensions. */
int screen_sprite_get_char_region(const sprite_t sprite, char* out, region_t region);
/* out must be wx * wy bytes long. Returns how many bytes were written from sprite data.
	If there are cells outside of the region provided, the result will still be in wx*wy dimensions. */
int screen_sprite_get_attrib_region(const sprite_t sprite, attribute_t* out, region_t region);

int screen_sprite_width(const sprite_t sprite);
int screen_sprite_height(const sprite_t sprite);
rgb_color_t screen_sprite_dirt_color(const sprite_t sprite);
void screen_sprite_set_dirt_color(sprite_t sprite, rgb_color_t dirt_color);