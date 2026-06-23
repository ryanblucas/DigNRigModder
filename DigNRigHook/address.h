/*
	address.h ~ RL
	Hard-coded addresses in the PE and game values
*/

#pragma once

#define REMOVE_STATE_SIZE_CHECK
#include "file.h"

#define ADDRESS_TEXT_WRITE_STATE			0x00038100
#define ADDRESS_TEXT_WRITE_STATE_LENGTH		0x07
#define ADDRESS_TEXT_WRITE_STATE_RETURN		0x00038107

#define ADDRESS_TEXT_LOAD_PROFILE			0x00037C44
#define ADDRESS_TEXT_LOAD_PROFILE_LENGTH	0x05
#define ADDRESS_TEXT_LOAD_PROFILE_RETURN	0x00037C49

#define ADDRESS_TEXT_RENDER_PROFILE_INFO			0x00024FB7
#define ADDRESS_TEXT_RENDER_PROFILE_INFO_LENGTH		0x06
#define ADDRESS_TEXT_RENDER_PROFILE_INFO_RETURN		0x00024FBD

#define ADDRESS_TEXT_GAME_MAKE_STARTING_RIG			0x000045C4
#define ADDRESS_TEXT_CHECK_INSIDE_EXIT_BOX			0x0002F04A
#define ADDRESS_TEXT_CHECK_INSIDE_EXIT_BOX_LENGTH	0x2E
#define ADDRESS_TEXT_DRAW_FACTORY_ANIMATION_CALL	0x00042626
#define ADDRESS_TEXT_DRAW_FACTORY_ANIMATION_CALL_LENGTH 0x2F

#define ADDRESS_FLOAT_START_X	0x000520EC
#define ADDRESS_FLOAT_START_Y	0x0005306C
#define ADDRESS_PLAYER			0x0034B520
#define ADDRESS_PTR_SCREEN_DATA	0x0065C5CC
#define ADDRESS_INT_SCREEN_DEPTH 0x0065E3E0

#define ADDRESS_GET_CONSTANT(type, addr) ((const type*)(address_base_pointer() + (addr)))

void address_initialize(void);

uintptr_t __cdecl address_base_pointer(void);

/* Acquires data from offset+location of size "size." This is necessary to write over data from Dig-N-Rig's executing space. Otherwise, use ADDRESS_GET_CONSTANT. */
void* address_acquire_data(uintptr_t location, size_t size);
/* Acquires the data pointed to from offset+location of size "size." */
void* address_acquire_ptr(uintptr_t location, size_t size);
/* Releases ptr returned from address_acquire_* */
void address_release_data(void* ptr);

/* Injects payload at location. */
void address_text_inject_payload(uintptr_t addr, const void* payload, size_t len);
/* Injects CALL opcode @ "Dig-N-Rig.exe"+addr' parameter to call whatever ptr is */
void address_text_inject_call(uintptr_t addr, uintptr_t func);
/* Writes length amount of NOPs at addr, then writes a JMP to func at addr. Be careful with
   this--all this function does on its own is overwrite max(length, 5) bytes at addr. */
void address_text_inject_code_cave(uintptr_t addr, uintptr_t func, size_t length);
/* Writes length amount of NOPs at addr */
void address_text_set_nop(uintptr_t addr, size_t length);

/* Gets the file name of the layer that is loaded from game_initialize_layers by index 0-(LAYER_COUNT - 1) */
const char* address_layer_filename_get(int index);
/* Sets the file name of the layer that is loaded to game_initialize_layers by index 0-(LAYER_COUNT - 1). */
void address_layer_filename_set(int index, const char* name);
/* Gets the name of the layer as seen in game from the function that shows it by index 0-(LAYER_COUNT - 1) */
const char* address_layer_name_get(int index);
/* Sets the name of the layer as seen in game to the function that shows it by index 0-(LAYER_COUNT - 1) */
void address_layer_name_set(int index, const char* name);