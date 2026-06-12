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

BOOL APIENTRY DllMain(HMODULE module, DWORD reason_for_call, LPVOID reserved)
{
    if (reason_for_call != DLL_PROCESS_ATTACH)
    {
        return TRUE; /* return result doesn't actually matter under these circumstances */
    }
    address_initialize();
    for (int i = 0; i < LAYER_COUNT; i++)
    {
        address_layer_filename_set(i, address_layer_filename_get(i));
        address_layer_name_set(i, "Hello World");
    }

    return TRUE;
}