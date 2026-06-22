/*
	address.c ~ RL
	Initializes addresses in address.h by adding base address, and other helper functions
*/

#include "address.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <Windows.h>

static uintptr_t base_address;

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

static struct access
{
	DWORD previous_permissions;
	uintptr_t location;
	size_t size;
	bool used;
} access_table[32];

void address_initialize(void)
{
	/* base_address can never be 0, so this removes a redundant variable to check for initialization */
	if (!base_address)
	{
		base_address = (uintptr_t)GetModuleHandleA(NULL);
	}
}

uintptr_t __cdecl address_base_pointer(void)
{
	return base_address;
}

static bool address_verify_access(uintptr_t location, size_t size)
{
	for (int i = 0; i < sizeof access_table / sizeof * access_table; i++)
	{
		if (access_table[i].used 
			&& (access_table[i].location < location && access_table[i].location + access_table[i].size > location)
			|| (access_table[i].location < location + size && access_table[i].location + access_table[i].size > location + size))
		{
			return false;
		}
	}
	return true;
}

void* address_acquire_data(uintptr_t location, size_t size)
{
	location += base_address;
	RUNTIME_ASSERT(address_verify_access(location, size));
	for (int i = 0; i < sizeof access_table / sizeof * access_table; i++)
	{
		if (!access_table[i].used)
		{
			VirtualProtect((LPVOID)location, size, PAGE_READWRITE, &access_table[i].previous_permissions);
			access_table[i].location = location;
			access_table[i].size = size;
			access_table[i].used = true;
			return (void*)location;
		}
	}
	return NULL;
}

void* address_acquire_ptr(uintptr_t location, size_t size)
{
	uintptr_t ptr = *ADDRESS_GET_CONSTANT(uintptr_t, location);
	return address_acquire_data(ptr - base_address, size);
}

void address_release_data(void* ptr)
{
	uintptr_t location = (uintptr_t)ptr;
	for (int i = 0; i < sizeof access_table / sizeof * access_table; i++)
	{
		if (access_table[i].used && access_table[i].location == location)
		{
			DWORD temp;
			VirtualProtect((LPVOID)location, access_table[i].size, access_table[i].previous_permissions, &temp);
			access_table[i].used = false;
			return;
		}
	}
}

void address_text_inject_payload(uintptr_t addr, const void* payload, size_t len)
{
	addr += base_address;

	DWORD previous, temp;
	VirtualProtect((LPVOID)addr, len, PAGE_EXECUTE_READWRITE, &previous);
	memcpy((void*)addr, payload, len);
	VirtualProtect((LPVOID)addr, len, previous, &temp);

	FlushInstructionCache((HANDLE)base_address, (LPCVOID)addr, len);
}

void address_text_inject_call(uintptr_t addr, uintptr_t func)
{
	addr += base_address;
	func -= addr + 5; /* relative call opcode calls the function offset from the next instruction */
	DWORD previous, temp;
	VirtualProtect((LPVOID)addr, 5, PAGE_EXECUTE_READWRITE, &previous);
	memset((void*)addr, 0xE8, 1);
	memcpy((void*)(addr + 1), &func, 4);
	VirtualProtect((LPVOID)addr, 5, previous, &temp);

#pragma warning(push)
#pragma warning(disable : 6385)
	FlushInstructionCache((HANDLE)base_address, (LPCVOID)addr, 5);
#pragma warning(pop)
}

void address_text_inject_code_cave(uintptr_t addr, uintptr_t func, size_t length)
{
	RUNTIME_ASSERT(length >= 5);

	addr += base_address;
	func -= addr + 5; /* rel32 is the function offset from the next instruction */

	DWORD previous, temp;
	VirtualProtect((LPVOID)addr, length, PAGE_EXECUTE_READWRITE, &previous);

	memset((void*)addr, 0x90, length); /* NOP */
	memset((void*)addr, 0xE9, 1); /* JMP rel32 */
	memcpy((void*)(addr + 1), &func, sizeof func); /* sets rel32 */

	VirtualProtect((LPVOID)addr, length, previous, &temp);

	FlushInstructionCache((HANDLE)base_address, (LPCVOID)addr, length);
}

void address_text_set_nop(uintptr_t addr, size_t length)
{
	addr += base_address;
	DWORD previous, temp;
	VirtualProtect((LPVOID)addr, length, PAGE_EXECUTE_READWRITE, &previous);
	memset((void*)addr, 0x90, length); /* NOP */
	VirtualProtect((LPVOID)addr, length, previous, &temp);
	FlushInstructionCache((HANDLE)base_address, (LPCVOID)addr, length);
}

const char* address_layer_filename_get(int index)
{
	RUNTIME_ASSERT(index >= 0 && index < LAYER_COUNT);
	return *(const char**)(base_address + text_layer_strings[index]);
}

void address_layer_filename_set(int index, const char* name)
{
	RUNTIME_ASSERT(index >= 0 && index < LAYER_COUNT);
	address_text_inject_payload(text_layer_strings[index], &name, sizeof name);
}

const char* address_layer_name_get(int index)
{
	RUNTIME_ASSERT(index >= 0 && index < LAYER_COUNT);
	return *(const char**)(base_address + text_layer_name_strings[index]);
}

void address_layer_name_set(int index, const char* name)
{
	RUNTIME_ASSERT(index >= 0 && index < LAYER_COUNT);
	address_text_inject_payload(text_layer_name_strings[index], &name, sizeof name);
}