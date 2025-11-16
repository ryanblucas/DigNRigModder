/*
	screen.h ~ RL
*/

#pragma once

#include "types.h"

typedef void (*screen_handle_repaint_t)();
typedef void (*screen_handle_left_arrow_t)();
typedef void (*screen_handle_right_arrow_t)();

typedef struct screen_events
{
	screen_handle_repaint_t repaint;
	screen_handle_left_arrow_t left;
	screen_handle_right_arrow_t right;
} screen_events_t;

void screen_initialize(screen_events_t events);
void screen_destroy(void);
void screen_loop(void);
void screen_repaint(void);
void screen_change_title(const char* title);

sprite_t screen_sprite_create(int width, int height, char* text, attribute_t* attrib);
void screen_sprite_destroy(sprite_t sprite);
void screen_sprite_render(int x, int y, sprite_t sprite);

int screen_sprite_width(sprite_t sprite);
int screen_sprite_height(sprite_t sprite);