/*
	address.c ~ RL
	Initializes addresses in address.h by adding base address, and other helper functions
*/

#include "address.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <Windows.h>

static bool initialized;

static uintptr_t text_layer_strings[LAYER_COUNT] =
{
	0x000044F5, /* luna */
	0x00004501, /* sta2_done */
	0x00004510, /* sta1_done */
	0x0000451F, /* blank */
	0x0000452E, /* surface */
	0x0000453D, /* caverns */
	0x0000454C, /* forest */
	0x0000455B, /* ruins */
	0x0000456D, /* whispy */
	0x0000457C, /* city */
	0x0000458B, /* treasuretemple */
	0x0000459A, /* dino_den */
	0x000045A9, /* magma */
	0x000045B8, /* core */
};

static uintptr_t text_layer_name_strings[LAYER_COUNT] =
{
	0x00018229, /* luna */
	0x00018241, /* sta2_done */
	0x0001824E, /* sta1_done */
	0x00018269, /* blank */
	0x00018281, /* surface */
	0x0001828E, /* caverns */
	0x000182A9, /* forest */
	0x000182C1, /* ruins */
	0x000182CB, /* whispy */
	0x000182E6, /* city */
	0x000182FE, /* treasuretemple */
	0x00018308, /* dino_den */
	0x00018323, /* magma */
	0x0001833B, /* core */
};

void address_initialize(void)
{
	if (initialized)
	{
		return;
	}
	uintptr_t base_address = (uintptr_t)GetModuleHandleA(NULL);
	for (int i = 0; i < LAYER_COUNT; i++)
	{
		text_layer_strings[i] += base_address;
		text_layer_name_strings[i] += base_address;
	}
	initialized = true;
}

const char* address_layer_filename_get(int index)
{
	assert(index >= 0 && index < LAYER_COUNT);
	return *(const char**)text_layer_strings[index];
}

static inline void address_text_set(uintptr_t addr, const void* value, size_t size)
{
	DWORD previous, temp;
	VirtualProtect(addr, size, PAGE_EXECUTE_READWRITE, &previous);
	memcpy(addr, value, size);
	VirtualProtect(addr, size, previous, &temp);
}

void address_layer_filename_set(int index, const char* name)
{
	assert(index >= 0 && index < LAYER_COUNT);
	address_text_set(text_layer_strings[index], &name, sizeof name);
}

const char* address_layer_name_get(int index)
{
	assert(index >= 0 && index < LAYER_COUNT);
	return *(const char**)text_layer_name_strings[index];
}

void address_layer_name_set(int index, const char* name)
{
	assert(index >= 0 && index < LAYER_COUNT);
	address_text_set(text_layer_name_strings[index], &name, sizeof name);
}