/*
	dllmain.c ~ RL
*/

#include "hook.h"
#include "address.h"
#include <io.h>
#include "path.h"
#include "select_campaign_state.h"
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

static campaign_t default_campaign;

static int current_profile = 0;
static campaign_t* campaigns[3];
static char campaign_directories[MAX_PATH * 3];

int hook_get_current_profile(void)
{
	const WCHAR* profile_name = *ADDRESS_GET_CONSTANT(WCHAR*, ADDRESS_PTR_PROFILE_ADDRESS);
	while (*profile_name && !(*profile_name >= L'0' && *profile_name <= L'9'))
	{
		profile_name++;
	}
	RUNTIME_ASSERT(*profile_name);
	current_profile = *profile_name - L'1';
	RUNTIME_ASSERT(current_profile >= 0 && current_profile <= 2);
	return current_profile;
}

static void hook_load_campaign(const campaign_t* camp)
{
	float* ptr = address_acquire_data(ADDRESS_FLOAT_START_X, sizeof * ptr);
	*ptr = camp->start_x;
	address_release_data(ptr);
	ptr = address_acquire_data(ADDRESS_FLOAT_START_Y, sizeof * ptr);
	*ptr = camp->start_y;
	address_release_data(ptr);

	for (int i = 0; i < 14; i++)
	{
		address_layer_filename_set(i, camp->layers[i].directory);
		address_layer_name_set(i, camp->layers[i].name);
	}
}

void hook_set_profile_campaign(int profile, const char* directory)
{
	if (directory)
	{
		strncpy(&campaign_directories[profile * MAX_PATH], directory, MAX_PATH);
		if (campaigns[profile] != &default_campaign)
		{
			file_campaign_unload(campaigns[profile]);
		}
		campaigns[profile] = file_campaign_load(directory);
	}
	else
	{
		campaigns[profile] = &default_campaign;
	}

	if (profile == current_profile)
	{
		hook_load_campaign(campaigns[profile]);
	}
}

/* The condition that checks whether the player won or not only checks if their x and y
   are greater than two values, meaning the check is not a rectangle. This changes that. */

static int __cdecl hook_win_check(void)
{
	const dnr_player_t* player = ADDRESS_GET_CONSTANT(dnr_player_t, ADDRESS_PLAYER);
	return region_is_inside(campaigns[current_profile]->end_box, (int)player->sprite.x, (int)player->sprite.y);
}

static void __declspec(naked) hook_win_check_code_cave(void)
{
	__asm
	{
		/* both registers EAX and EDX are assigned before they are accessed in the function this code cave is in */

		call address_base_pointer
		mov edx, eax
		add edx, 0x2F078

		push edx
		call hook_win_check
		test eax, eax
		pop edx
		jnz jump_to
		add edx, 0xEF
	jump_to:
		jmp edx
	}
}

struct profile
{
	bool exists;
	int diggit_version;
	int current_layer;
	int seconds_spent;
	double completion_percent;
	int times_won;
};

static void __cdecl hook_profile_render(int num, struct profile* profile, int x, int y)
{
	if (y < 0 || y >= TARGET_HEIGHT || !campaigns[num])
	{
		return;
	}

	CHAR_INFO* screen_data = address_acquire_ptr(ADDRESS_PTR_SCREEN_DATA, sizeof * screen_data * TARGET_WIDTH * TARGET_HEIGHT);

	char msg[64];
	int len = snprintf(msg, sizeof msg, "%s", campaigns[num]->name);
	x -= len / 2;
	for (int i = 0; msg[i] != '\0' && x + i < TARGET_WIDTH; i++)
	{
		int pos = (y + 1) * TARGET_WIDTH + x + i;
		screen_data[pos].Attributes = CREATE_ATTRIBUTE(LIGHT_WHITE, DARK_BLACK);
		screen_data[pos].Char.AsciiChar = msg[i];
	}

	address_release_data(screen_data);
}

static void __declspec(naked) hook_profile_render_code_cave(void)
{
	__asm
	{
		call address_base_pointer
		add ecx, dword ptr[eax + 0x053A80] /* replicate overwritten behavior */

		push eax /* preserve state */
		push edx /* preserve state */
		push ecx /* y reg */
		push ebx /* x reg */

		/* profile pointer */
		mov ecx, dword ptr[esp + 0x28]
		sub ecx, 0x0C
		push ecx

		/* calculate profile index */
		mov ecx, dword ptr[esp + 0x28]
		sub ecx, eax
		sub ecx, 0x34A4CC
		shr ecx, 2
		push ecx

		call hook_profile_render
		add esp, 0x8
		pop ebx
		pop ecx

		pop edx
		pop eax

		add eax, ADDRESS_TEXT_RENDER_PROFILE_INFO_RETURN
		jmp eax
	}
}

static void __cdecl hook_load_profile(const char* filename)
{
	/* you can't actually use the file directly from the function because you don't have the permissions. so, this hooks before dnr opens the file */
	FILE* file = fopen(filename, "rb");

	while (*filename && !(*filename >= '0' && *filename <= '9'))
	{
		filename++;
	}
	RUNTIME_ASSERT(*filename);
	int profile_index = *filename - '1';

	campaigns[profile_index] = &default_campaign;

	char payload[MAX_PATH + 3];
	fseek(file, -(int)(sizeof payload), SEEK_END);
	if (fread(payload, 1, sizeof payload, file) != sizeof payload)
	{
		fclose(file);
		return;
	}
	if (payload[0] == 'M' && payload[1] == 'O' && payload[2] == 'D' && path_exists(payload + 3))
	{
		snprintf(&campaign_directories[profile_index * MAX_PATH], MAX_PATH, "%s", payload + 3);
		campaigns[profile_index] = file_campaign_load(&campaign_directories[profile_index * MAX_PATH]);
	}

	fclose(file);
}

static void __declspec(naked) hook_load_profile_code_cave(void)
{
	__asm
	{
		call address_base_pointer
		mov esi, eax
		add esi, ADDRESS_TEXT_LOAD_PROFILE_RETURN
		
		/* replicate overwritten behavior */
		lea eax, [esp + 0x14]
		push eax

		push ecx
		push edx
		call hook_load_profile
		pop edx
		pop ecx

		jmp esi
	}
}

static void __cdecl hook_write_state(const char* filename)
{
	if (campaigns[current_profile] == &default_campaign)
	{
		return;
	}

	/* you can't actually use the file directly from the function because you don't have the permissions. so, this hooks after dnr writes to the file and closes it */
	FILE* file = fopen(filename, "ab+");

	char begin[MAX_PATH + 3];
	fseek(file, -(long)(sizeof begin), SEEK_END);
	fread(begin, 1, sizeof begin, file);

	char payload[MAX_PATH + 3];
	memset(payload, 0, sizeof payload);
	payload[0] = 'M';
	payload[1] = 'O';
	payload[2] = 'D';
	/* no fprintf to the file directly, the payload at the end of the file needs to be the exact same so its easier to see if the payload exists or not when reading it back */
	snprintf(payload + 3, MAX_PATH, "%s", &campaign_directories[current_profile * MAX_PATH]);
	if (memcmp(begin, payload, sizeof payload) != 0)
	{
		RUNTIME_ASSERT(fwrite(payload, 1, sizeof payload, file) == sizeof payload);
	}
	fclose(file);
}

static void __declspec(naked) hook_write_state_code_cave(void)
{
	__asm
	{
		call address_base_pointer
		add eax, ADDRESS_TEXT_WRITE_STATE_RETURN
		push eax

		lea ecx, dword ptr[ebp - 0x110]
		push ecx
		call hook_write_state
		add esp, 0x4
		pop eax

		/* replicate overwritten behavior */
		mov ecx, dword ptr[esp + 0x10BC]

		jmp eax
	}
}

static void hook_load_existing_state(void)
{
	int profile = hook_get_current_profile();
	hook_load_campaign(campaigns[profile]);
	ADDRESS_CALL_DESTROY_STALACTITES();
	ADDRESS_CALL_DESTROY_LIQUIDS();
	ADDRESS_CALL_INITIALIZE_LAYERS();
	ADDRESS_CALL_LOAD_STATE();
}

static DWORD WINAPI hook_initialize(LPVOID param)
{
	default_campaign.end_box.x0 = 142;
	default_campaign.end_box.y0 = 1392;
	default_campaign.end_box.x1 = 150;
	default_campaign.end_box.y1 = 1400;
	default_campaign.name = "Dig-N-Rig";
	default_campaign.start_x = 46;
	default_campaign.start_y = 450;

	address_initialize();
	sce_initialize();

	for (int i = 0; i < 14; i++)
	{
		default_campaign.layers[i].name = address_layer_name_get(i);
		default_campaign.layers[i].directory = address_layer_filename_get(i);
	}
	for (int i = 0; i < 3; i++)
	{
		campaigns[i] = &default_campaign;
	}

	address_text_inject_code_cave(ADDRESS_TEXT_CHECK_INSIDE_EXIT_BOX, (uintptr_t)hook_win_check_code_cave, ADDRESS_TEXT_CHECK_INSIDE_EXIT_BOX_LENGTH);
	address_text_inject_code_cave(ADDRESS_TEXT_RENDER_PROFILE_INFO, (uintptr_t)hook_profile_render_code_cave, ADDRESS_TEXT_RENDER_PROFILE_INFO_LENGTH);
	address_text_inject_code_cave(ADDRESS_TEXT_LOAD_PROFILE, (uintptr_t)hook_load_profile_code_cave, ADDRESS_TEXT_LOAD_PROFILE_LENGTH);
	address_text_inject_code_cave(ADDRESS_TEXT_WRITE_STATE, (uintptr_t)hook_write_state_code_cave, ADDRESS_TEXT_WRITE_STATE_LENGTH);
	address_text_inject_call(ADDRESS_TEXT_GAME_START_SAVE, (uintptr_t)hook_load_existing_state);
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