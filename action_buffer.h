/*
	action_buffer.h ~ RL
	Buffers actions for undo/redo functionality
*/

#pragma once

#include "file.h"
#include "types.h"

typedef struct complete_block
{
	dnr_block_t block;
	dnr_mineral_t mineral;
	stalactite_t stalactite;
} complete_block_t;

typedef struct complete_field
{
	size_t field_size;
	void* value;
} complete_field_t;

typedef enum action_type
{
	ACTION_BLOCK,
	ACTION_FIELD
} action_type_t;

/* Dig-N-Rig is a 32-bit application, meaning all elementary fields (like CHAR_INFO 
   and ints, not something like dnr_save_header_t) are at least 4 bytes. */
typedef uint32_t field_t;

typedef struct action_block
{
	region_t region;
	complete_block_t* previous;
	complete_block_t* next;
} action_block_t;

typedef struct action_field
{
	size_t size;
	void* ptr;
	field_t previous;
	field_t next;
} action_field_t;

typedef struct action
{
	action_type_t type;
	union
	{
		action_block_t block;
		action_field_t field;
	} sub;
} action_t;

extern inline field_t field_create(void* ptr, size_t size)
{
	field_t res;
	RUNTIME_ASSERT(size <= sizeof res);
	if (size == 4)
	{
		res = *(uint32_t*)ptr;
	}
	else if (size == 2)
	{
		res = *(uint16_t*)ptr;
	}
	else
	{
		res = *(uint8_t*)ptr;
	}
	return res;
}

void action_buffer_initialize(void);
void action_buffer_destroy(void);

/* Adds block action to buffer. The action created uses the region parameter passed in
   and creates the "previous" array with what is there currently. Therefore, call this
   before changing anything, then action_buffer_post_add_block to create the next part.
   If this isn't the top of the buffer, it deletes everything in front of it */
void action_buffer_pre_add_block(const dnr_state_t* state, region_t region);
/* Finalizes block action to buffer from pre_add_block. */
void action_buffer_post_add_block(const dnr_state_t* state);
/* Adds field action to buffer. All of the parameters passed in are 1:1 what is created
   for the action_field_t struct. If this isn't the top of the buffer, it deletes everything in front of it */
void action_buffer_add_field(size_t size, void* ptr, field_t previous);
/* Goes back in the buffer. If there's no more left, returns false and doesn't write to action.
   This would be used for undoing */
action_t* action_buffer_back(void);
/* Goes forward in the buffer. If there's no more left, returns false and doesn't write to action.
   This would be used for redoing */
action_t* action_buffer_forward(void);
/* Reverses a block action */
void action_buffer_reverse_block(dnr_state_t* state, action_t* action);
/* Reverses a field action */
void action_buffer_reverse_field(action_t* action);