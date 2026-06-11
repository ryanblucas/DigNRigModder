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
	}
	initialized = true;
}

const char* address_layer_filename_get(int index)
{
	assert(index >= 0 && index < LAYER_COUNT);
	return *(const char**)text_layer_strings[index];
}

void address_layer_filename_set(int index, const char* name)
{
	DWORD previous;
	VirtualProtect(text_layer_strings[index], sizeof(uintptr_t), PAGE_EXECUTE_READWRITE, &previous);

	*(const char**)text_layer_strings[index] = name;

	DWORD temp;
	VirtualProtect(text_layer_strings[index], sizeof(uintptr_t), previous, &temp);
}