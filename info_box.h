/*
	info_box.h ~ RL
	Window that increases interactivity with the main display
*/

#pragma once

typedef struct info_section
{
	char* title;
} info_section_t;

void info_initialize(const info_section_t* section_array, int section_count);
void info_destroy(void);