/*
	string_builder.h ~ RL
	This is more of a utility to make a string array, not like what you'd see in Java or C# under the same name
*/

#pragma once

#include "types.h"

typedef struct string_builder
{
	char* buf;
	char* ptr;
	size_t capacity;
} string_builder_t;

string_builder_t* string_builder_create(size_t start_capacity);
void string_builder_destroy(string_builder_t* builder);

/* adds str to builder with NULL terminator, returns offset from buf of where this is */
size_t string_builder_add(string_builder_t* builder, const char* str);