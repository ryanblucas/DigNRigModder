/*
	action_buffer.c ~ RL
	Buffers actions for undo/redo functionality
*/

#include "action_buffer.h"

static action_t buffer[64];
static int position = -1, size = 0;

void action_buffer_initialize(void)
{

}

void action_buffer_destroy(void)
{
	for (int i = 0; i < size; i++)
	{
		if (buffer[i].type == ACTION_BLOCK)
		{
			free(buffer[i].sub.block.previous);
			free(buffer[i].sub.block.next);
		}
	}
}

static void action_buffer_create_node(void)
{
	position++;
	for (int i = position; i < size; i++)
	{
		if (buffer[i].type == ACTION_BLOCK)
		{
			free(buffer[i].sub.block.previous);
			free(buffer[i].sub.block.next);
		}
	}
	size = position + 1;
	if (size >= sizeof buffer / sizeof * buffer)
	{
		memmove(buffer, buffer + 1, sizeof buffer - sizeof * buffer);
		size = sizeof buffer / sizeof * buffer;
	}
}

static void action_buffer_copy_region(const dnr_state_t* state, region_t region, complete_block_t* arr)
{
	memset(arr, 0, region_size(region) * sizeof * arr);
	for (int y = 0; y < region_height(region); y++)
	{
		for (int x = 0; x < region_width(region); x++)
		{
			int index = y * region_width(region) + x;
			int state_index = (region.x0 + x) * WORLD_HEIGHT + (region.y0 + y);
			arr[index].block = state->blocks[state_index];
			if (state->blocks[state_index].mineral_exists)
			{
				RUNTIME_ASSERT(state->blocks[state_index].mineral_index >= 0 && state->blocks[state_index].mineral_index < sizeof state->minerals / sizeof * state->minerals);
				arr[index].mineral = state->minerals[state->blocks[state_index].mineral_index];
			}
			for (int i = 0; i < state->stalactite_count; i++)
			{
				if (state->stalactite_array[i].exists && (int)state->stalactite_array[i].x == region.x0 + x && (int)state->stalactite_array[i].y == region.y0 + y)
				{
					arr[index].stalactite = state->stalactite_array[i];
					break;
				}
			}
		}
	}
}

void action_buffer_pre_add_block(const dnr_state_t* state, region_t region)
{
	action_buffer_create_node();

	buffer[position].type = ACTION_BLOCK;

	action_block_t* block_action = &buffer[position].sub.block;
	block_action->region = region;
	block_action->previous = dig_malloc(region_size(region) * sizeof * block_action->previous);
	action_buffer_copy_region(state, region, block_action->previous);
}

void action_buffer_post_add_block(const dnr_state_t* state)
{
	RUNTIME_ASSERT(buffer[position].type == ACTION_BLOCK);

	action_block_t* block_action = &buffer[position].sub.block;
	block_action->next = dig_malloc(region_size(block_action->region) * sizeof * block_action->next);
	action_buffer_copy_region(state, block_action->region, block_action->next);
}

void action_buffer_add_field(element_t element, field_t previous)
{
	action_buffer_create_node();

	buffer[position].type = ACTION_FIELD;
	buffer[position].sub.field = (action_field_t)
	{
		.next = field_create(serialize_element_get_value(element), serialize_element_get_size(element)),
		.previous = previous,
		.element = element
	};
}

action_t* action_buffer_back(void)
{
	if (position < 0)
	{
		return NULL;
	}

	return &buffer[position--];
}

action_t* action_buffer_forward(void)
{
	if (position >= size - 1)
	{
		return false;
	}
	
	return &buffer[++position];
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