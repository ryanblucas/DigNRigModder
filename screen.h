/*
	screen.h ~ RL
*/

#pragma once

#include "types.h"

typedef int virtual_key_t;
#ifndef _WIN_USER_
/* #define NOVIRTUALKEYCODES */

#define VK_BACK           0x08
#define VK_TAB            0x09

/*
 * 0x0A - 0x0B : reserved
 */

#define VK_CLEAR          0x0C
#define VK_RETURN         0x0D

 /*
  * 0x0E - 0x0F : unassigned
  */

#define VK_SHIFT          0x10
#define VK_CONTROL        0x11
#define VK_MENU           0x12
#define VK_PAUSE          0x13
#define VK_CAPITAL        0x14

#define VK_ESCAPE         0x1B

#define VK_SPACE          0x20
#define VK_PRIOR          0x21
#define VK_NEXT           0x22
#define VK_END            0x23
#define VK_HOME           0x24
#define VK_LEFT           0x25
#define VK_UP             0x26
#define VK_RIGHT          0x27
#define VK_DOWN           0x28
#define VK_SELECT         0x29
#define VK_PRINT          0x2A
#define VK_EXECUTE        0x2B
#define VK_SNAPSHOT       0x2C
#define VK_INSERT         0x2D
#define VK_DELETE         0x2E
#define VK_HELP           0x2F

  /*
   * VK_0 - VK_9 are the same as ASCII '0' - '9' (0x30 - 0x39)
   * 0x3A - 0x40 : unassigned
   * VK_A - VK_Z are the same as ASCII 'A' - 'Z' (0x41 - 0x5A)
   */

#define VK_LWIN           0x5B
#define VK_RWIN           0x5C
#define VK_APPS           0x5D

   /*
	* 0x5E : reserved
	*/

#define VK_SLEEP          0x5F

#define VK_NUMPAD0        0x60
#define VK_NUMPAD1        0x61
#define VK_NUMPAD2        0x62
#define VK_NUMPAD3        0x63
#define VK_NUMPAD4        0x64
#define VK_NUMPAD5        0x65
#define VK_NUMPAD6        0x66
#define VK_NUMPAD7        0x67
#define VK_NUMPAD8        0x68
#define VK_NUMPAD9        0x69
#define VK_MULTIPLY       0x6A
#define VK_ADD            0x6B
#define VK_SEPARATOR      0x6C
#define VK_SUBTRACT       0x6D
#define VK_DECIMAL        0x6E
#define VK_DIVIDE         0x6F
#define VK_F1             0x70
#define VK_F2             0x71
#define VK_F3             0x72
#define VK_F4             0x73
#define VK_F5             0x74
#define VK_F6             0x75
#define VK_F7             0x76
#define VK_F8             0x77
#define VK_F9             0x78
#define VK_F10            0x79
#define VK_F11            0x7A
#define VK_F12            0x7B
#define VK_F13            0x7C
#define VK_F14            0x7D
#define VK_F15            0x7E
#define VK_F16            0x7F
#define VK_F17            0x80
#define VK_F18            0x81
#define VK_F19            0x82
#define VK_F20            0x83
#define VK_F21            0x84
#define VK_F22            0x85
#define VK_F23            0x86
#define VK_F24            0x87

#define VK_NUMLOCK        0x90
#define VK_SCROLL         0x91

#endif

typedef void (*screen_handle_repaint_t)();
typedef void (*screen_handle_key_t)(virtual_key_t);

typedef struct screen_events
{
	screen_handle_repaint_t repaint;
	screen_handle_key_t keyboard;
} screen_events_t;

void screen_initialize(screen_events_t events);
void screen_destroy(void);
void screen_loop(void);
void screen_repaint(void);

void screen_change_title(const char* title);
void screen_change_color_palette(int id);

sprite_t screen_sprite_create(int width, int height, char* text, attribute_t* attrib);
void screen_sprite_destroy(sprite_t sprite);
void screen_sprite_render(int x, int y, sprite_t sprite);

int screen_sprite_width(sprite_t sprite);
int screen_sprite_height(sprite_t sprite);