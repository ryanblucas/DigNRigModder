/*
    dllmain.c ~ RL
*/

#include "address.h"
#include <stdio.h>
#include <Windows.h>

/* this is to resolve any linking issues with exports.def */

void __stdcall FMOD_System_Update() {}
void __stdcall FMOD_System_Create() {}
void __stdcall FMOD_System_GetVersion() {}
void __stdcall FMOD_System_Init() {}
void __stdcall FMOD_Channel_SetVolume() {}
void __stdcall FMOD_Channel_Stop() {}
void __stdcall FMOD_Channel_GetVolume() {}
void __stdcall FMOD_System_PlaySound() {}
void __stdcall FMOD_System_CreateSound() {}
void __stdcall FMOD_Channel_IsPlaying() {}

/* The condition that checks whether the player won or not only checks if their x and y
   are greater than two values, meaning the check is not a rectangle. This changes that. */

static int __cdecl hook_win_check(void)
{
    const dnr_player_t* player = ADDRESS_GET_CONSTANT(dnr_player_t, ADDRESS_PLAYER);
    /* original check for now */
    return 1392.0 < player->sprite.y && 142.0 < player->sprite.x;
}

static void __declspec(naked) hook_win_check_code_cave(void)
{
    __asm
    {
        /* both registers EAX and EDX are assigned before they are accessed in the function this code cave is in */

        call address_base_pointer
        mov edx, eax
        add edx, 0x2F078

        call hook_win_check
        test eax, eax
        jnz jump_to
        add edx, 0xEF
    jump_to:
        jmp edx
    }
}

static DWORD WINAPI hook_initialize(LPVOID param)
{
    address_initialize();
    address_text_inject_code_cave(ADDRESS_TEXT_CHECK_INSIDE_EXIT_BOX, (uintptr_t)hook_win_check_code_cave, ADDRESS_TEXT_CHECK_INSIDE_EXIT_BOX_LENGTH);
    
    //address_text_inject_call(ADDRESS_TEXT_GAME_MAKE_STARTING_RIG, (uintptr_t)hook_start_rig);
    
    //ADDRESS_ASSIGN_MEMORY(float, ADDRESS_FLOAT_START_X, 100.0F);
    //ADDRESS_ASSIGN_MEMORY(float, ADDRESS_FLOAT_START_Y, 300.0F);
    
    //address_text_set_nop(ADDRESS_TEXT_DRAW_FACTORY_ANIMATION_CALL, ADDRESS_TEXT_DRAW_FACTORY_ANIMATION_CALL_LENGTH);
    //address_text_inject_call(ADDRESS_TEXT_DRAW_FACTORY_ANIMATION_CALL, (uintptr_t)hook_draw_factory_animation);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason_for_call, LPVOID reserved)
{
    if (reason_for_call != DLL_PROCESS_ATTACH)
    {
        return TRUE; /* return result doesn't actually matter under these circumstances */
    }
    /* prevents deadlocking loader */
    CreateThread(NULL, 0, hook_initialize, NULL, 0, NULL);
    return TRUE;
}