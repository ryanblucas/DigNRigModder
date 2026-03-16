/*
	action_buffer.c ~ RL
	Buffers actions for undo/redo functionality
*/

#include "action_buffer.h"

struct action_buffer
{
	action_t buffer[64];
	int position, size;
};

action_buffer_t action_buffer_initialize(void)
{
	action_buffer_t result = dig_malloc(sizeof * result);
	memset(result, 0, sizeof * result);
	result->position = -1;
	return result;
}

void action_buffer_destroy(action_buffer_t buffer)
{
	for (int i = 0; i < buffer->size; i++)
	{
		if (buffer->buffer[i].type == ACTION_BLOCK)
		{
			free(buffer->buffer[i].sub.block.previous);
			free(buffer->buffer[i].sub.block.next);
		}
	}
}

static void action_buffer_create_node(action_buffer_t buffer)
{
	buffer->position++;
	for (int i = buffer->position; i < buffer->size; i++)
	{
		if (buffer->buffer[i].type == ACTION_BLOCK)
		{
			free(buffer->buffer[i].sub.block.previous);
			free(buffer->buffer[i].sub.block.next);
		}
	}
	buffer->size = buffer->position + 1;
	if (buffer->size >= sizeof buffer->buffer / sizeof * buffer->buffer)
	{
		memmove(buffer->buffer, buffer->buffer + 1, sizeof buffer->buffer - sizeof * buffer->buffer);
		buffer->size = sizeof buffer->buffer / sizeof * buffer->buffer;
	}
}

void action_buffer_pre_add_block(action_buffer_t buffer, const dnr_state_t* state, region_t region)
{
	action_buffer_create_node(buffer);

	buffer->buffer[buffer->position].type = ACTION_BLOCK;

	action_block_t* block_action = &buffer->buffer[buffer->position].sub.block;
	block_action->region = region;
	block_action->previous = dig_malloc(region_size(region) * sizeof * block_action->previous);
	game_copy(state, region, block_action->previous);
}

void action_buffer_post_add_block(action_buffer_t buffer, const dnr_state_t* state)
{
	RUNTIME_ASSERT(buffer->buffer[buffer->position].type == ACTION_BLOCK);

	action_block_t* block_action = &buffer->buffer[buffer->position].sub.block;
	block_action->next = dig_malloc(region_size(block_action->region) * sizeof * block_action->next);
	game_copy(state, block_action->region, block_action->next);
}

void action_buffer_add_block(action_buffer_t buffer, const complete_block_t* prev, const complete_block_t* curr, region_t region)
{
	action_buffer_create_node(buffer);
	buffer->buffer[buffer->position].type = ACTION_BLOCK;
	action_block_t* block_action = &buffer->buffer[buffer->position].sub.block;
	block_action->region = region;

	size_t buf_size = region_size(region) * sizeof * prev;
	block_action->previous = dig_malloc(buf_size);
	block_action->next = dig_malloc(buf_size);
	memcpy(block_action->previous, prev, buf_size);
	memcpy(block_action->next, curr, buf_size);
}

void action_buffer_add_field(action_buffer_t buffer, element_t element, field_t previous)
{
	field_t next = field_create(serialize_element_get_value(element), serialize_element_get_size(element));
	if (next == previous)
	{
		return;
	}
	action_buffer_create_node(buffer);

	buffer->buffer[buffer->position].type = ACTION_FIELD;
	buffer->buffer[buffer->position].sub.field = (action_field_t)
	{
		.next = next,
		.previous = previous,
		.element = element
	};
}

action_t* action_buffer_back(action_buffer_t buffer)
{
	if (buffer->position < 0)
	{
		return NULL;
	}

	return &buffer->buffer[buffer->position--];
}

action_t* action_buffer_forward(action_buffer_t buffer)
{
	if (buffer->position >= buffer->size - 1)
	{
		return false;
	}
	
	return &buffer->buffer[++buffer->position];
}

void action_buffer_reverse_block(dnr_state_t* state, action_t* action)
{
	RUNTIME_ASSERT(action->type == ACTION_BLOCK);
	action_block_t* ba = &action->sub.block;
	for (int i = 0; i < region_size(ba->region); i++)
	{
		int x = i % region_width(ba->region) + ba->region.x0;
		int y = i / region_width(ba->region) + ba->region.y0;
		int j = x * WORLD_HEIGHT + y;
		state->blocks[j] = ba->previous[i].block;
		if (state->blocks[j].mineral_exists)
		{
			RUNTIME_ASSERT(state->blocks[j].mineral_index >= 0 && state->blocks[j].mineral_index < sizeof state->minerals / sizeof * state->minerals);
			state->minerals[state->blocks[j].mineral_index] = ba->previous[i].mineral;
		}
		if (ba->next[i].stalactite.exists)
		{
			for (int k = 0; k < state->stalactite_count; k++)
			{
				if (state->stalactite_array[k].x == ba->next[i].stalactite.x && state->stalactite_array[k].y == ba->next[i].stalactite.y)
				{
					state->stalactite_array[k].exists = false;
				}
			}
		}
		if (ba->previous[i].stalactite.exists)
		{
			for (int k = 0; k < state->stalactite_count; k++)
			{
				if (!state->stalactite_array[k].exists)
				{
					state->stalactite_array[k] = ba->previous[i].stalactite;
					break;
				}
			}
		}
	}
	complete_block_t* temp = ba->previous;
	ba->previous = ba->next;
	ba->next = temp;
}

void action_buffer_reverse_field(action_t* action)
{
	RUNTIME_ASSERT(action->type == ACTION_FIELD);
	action_field_t* field = &action->sub.field;
	serialize_element_set_value(field->element, &field->previous);
	field_t temp = field->previous;
	field->previous = field->next;
	field->next = temp;
}