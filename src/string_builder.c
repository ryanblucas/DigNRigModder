/*
	string_builder.c ~ RL
*/

#include "string_builder.h"
#include <string.h>

string_builder_t* string_builder_create(size_t start_capacity)
{
	string_builder_t* result = dig_malloc(sizeof * result);
	result->buf = dig_malloc(start_capacity);
	result->ptr = result->buf;
	result->capacity = start_capacity;
	return result;
}

void string_builder_destroy(string_builder_t* builder)
{
	free(builder->buf);
	free(builder);
}

size_t string_builder_add(string_builder_t* builder, const char* str)
{
	size_t result = builder->ptr - builder->buf;
	str--;
	do
	{
		str++;
		*builder->ptr = *str;
		builder->ptr++;
	} while (*str && builder->ptr < builder->buf + builder->capacity);

	if (builder->ptr >= builder->buf + builder->capacity)
	{
		char* next = dig_malloc(builder->capacity * 2);
		memcpy(next, builder->buf, builder->capacity);
		free(builder->buf);
		builder->buf = next;
		builder->capacity *= 2;
		string_builder_add(builder, str + 1);
	}

	return result;
}