/*
	select_campaign_state.c ~ RL
	Select campaign state, started after opening an empty save slot
*/

#include "select_campaign_state.h"
#include "address.h"

static void __declspec(naked) sce_on_create_save(void)
{
	__asm
	{
		call scs_start
		call address_base_pointer
		add eax, ADDRESS_TEXT_CREATE_STATE_RETURN
		jmp eax
	}
}

static void __declspec(naked) sce_on_state_switch(void)
{
	__asm
	{
		mov ecx, eax
		call address_base_pointer
		add eax, ADDRESS_TEXT_STATE_SWITCH_DEFAULT
		cmp ecx, 0x4
		jne jump_to_end

		push eax
		call scs_update
		mov ecx, eax
		pop eax

		test ecx, ecx
		jnz jump_to_end
		sub eax, (ADDRESS_TEXT_STATE_SWITCH_DEFAULT - ADDRESS_TEXT_CREATE_STATE_WORK)
		jump_to_end:
		jmp eax
	}
}

void sce_initialize(void)
{
	address_text_inject_code_cave(ADDRESS_TEXT_CREATE_STATE, (uintptr_t)sce_on_create_save, ADDRESS_TEXT_CREATE_STATE_LENGTH);
	uintptr_t ptr = (uintptr_t)sce_on_state_switch - (address_base_pointer() + ADDRESS_TEXT_JA_SWITCH_STATE + 0x06);
	address_text_inject_payload(ADDRESS_TEXT_JA_SWITCH_STATE + 0x02, &ptr, sizeof ptr);
}

void __cdecl scs_start(void)
{
	int* app_state = address_acquire_data(ADDRESS_INT_APP_STATE, sizeof * app_state);
	*app_state = 4;
	address_release_data(app_state);
}

enum scs_result __cdecl scs_update(void)
{
	/* to do: menu music stops playing, JMP to the code that runs the music in
       main_menu_render and put a code cave after to JE back if state == 4 */
	/* to do: animate dig-n-rig logo in the back */

	if (*ADDRESS_GET_CONSTANT(bool, ADDRESS_BOOL_UP_ARROW_STATE))
	{
		int* app_state = address_acquire_data(ADDRESS_INT_APP_STATE, sizeof * app_state);
		*app_state = 0;
		address_release_data(app_state);
		return SCS_RESULT_CREATE_STATE;
	}
	else if (*ADDRESS_GET_CONSTANT(bool, ADDRESS_BOOL_DOWN_ARROW_STATE))
	{
		int* app_state = address_acquire_data(ADDRESS_INT_APP_STATE, sizeof * app_state);
		*app_state = 1;
		address_release_data(app_state);
	}

	return SCS_RESULT_CONTINUE;
}