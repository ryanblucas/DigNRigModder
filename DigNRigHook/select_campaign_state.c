/*
	select_campaign_state.c ~ RL
	Select campaign state, started after opening an empty save slot
*/

#include "select_campaign_state.h"
#include "address.h"
#include "hook.h"
#include "path.h"

#define DEFAULT_CAMPAIGN_NAME "Dig-N-Rig"
#define WINDOW_TITLE "Select"

static char* campaign_names_array;
static int campaign_names_length;
static int selection_index;
static int longest_name_length;

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

	selection_index = 0;
	longest_name_length = 0;
	campaign_names_length = 0;
	free(campaign_names_array);

	char* temp = path_enumerate_directory_create("*.campaign", &campaign_names_length);
	char* curr = temp;
	int char_len = sizeof DEFAULT_CAMPAIGN_NAME;
	for (int i = 0; i < campaign_names_length; i++)
	{
		int len = (int)strnlen(curr, MAX_PATH);
		char_len += len + 1;
		curr += len + 1;
		longest_name_length = max(len, longest_name_length);
	}
	campaign_names_array = dig_malloc(char_len);
	strncpy(campaign_names_array, DEFAULT_CAMPAIGN_NAME, sizeof DEFAULT_CAMPAIGN_NAME);
	memcpy(campaign_names_array + sizeof DEFAULT_CAMPAIGN_NAME, temp, char_len - sizeof DEFAULT_CAMPAIGN_NAME);
	free(temp);
	campaign_names_length++; /* for the dig-n-rig default */
}

static inline scs_set_ci_safe(CHAR_INFO* screen, CHAR_INFO ci, int x, int y)
{
	if (x >= 0 && x < TARGET_WIDTH && y >= 0 && y < TARGET_HEIGHT)
	{
		screen[y * TARGET_WIDTH + x] = ci;
	}
}

static void scs_render_border(CHAR_INFO* screen, SHORT attribs, int x, int y, int width, int height)
{
	int cx = max(0, x);
	int cwx = min(TARGET_WIDTH, width + x) - cx;
	int cy = max(0, y);
	int cwy = min(TARGET_HEIGHT, height + y) - cy;

	for (int i = cx; i < cx + cwx; i++)
	{
		CHAR_INFO ci = { .Attributes = attribs, .Char.AsciiChar = 0xCD };
		screen[cy * TARGET_WIDTH + i] = ci;
		screen[(cy + cwy - 1) * TARGET_WIDTH + i] = ci;
	}
	for (int i = cy; i < cy + cwy; i++)
	{
		CHAR_INFO ci = { .Attributes = attribs, .Char.AsciiChar = 0xBA };
		screen[i * TARGET_WIDTH + cx] = ci;
		screen[i * TARGET_WIDTH + cx + cwx - 1] = ci;
	}
	scs_set_ci_safe(screen, (CHAR_INFO) { .Attributes = attribs, .Char.AsciiChar = 0xC9 }, x, y);
	scs_set_ci_safe(screen, (CHAR_INFO) { .Attributes = attribs, .Char.AsciiChar = 0xBB }, x + width - 1, y);
	scs_set_ci_safe(screen, (CHAR_INFO) { .Attributes = attribs, .Char.AsciiChar = 0xC8 }, x, y + height - 1);
	scs_set_ci_safe(screen, (CHAR_INFO) { .Attributes = attribs, .Char.AsciiChar = 0xBC }, x + width - 1, y + height - 1);
}

static inline void scs_render_rectangle(CHAR_INFO* screen, CHAR_INFO ci, int x, int y, int width, int height)
{
	for (int i = max(0, y); i < min(y + height, TARGET_HEIGHT); i++)
	{
		for (int j = max(0, x); j < min(x + width, TARGET_WIDTH); j++)
		{
			screen[i * TARGET_WIDTH + j] = ci;
		}
	}
}

static int scs_render_text(CHAR_INFO* screen, const char* str, int x, int y)
{
	if (y < 0 || y >= TARGET_HEIGHT)
	{
		return 0;
	}
	const char* start = str;
	while (x < TARGET_WIDTH && *str)
	{
		screen[y * TARGET_WIDTH + x].Char.AsciiChar = *str;
		x++;
		str++;
	}
	while (*str)
	{
		str++;
	}
	return (int)(str - start);
}

static char* scs_get_campaign_directory_by_index(int in)
{
	char* curr = campaign_names_array;
	for (int i = 0; i < in; i++)
	{
		int len = (int)strnlen(curr, MAX_PATH);
		curr += len + 1;
		longest_name_length = max(len, longest_name_length);
	}
	return curr;
}

enum scs_result __cdecl scs_update(void)
{
	/* to do: menu music stops playing, JMP to the code that runs the music in
       main_menu_render and put a code cave after to JE back if state == 4 */
	/* to do: animate dig-n-rig logo in the back */

	if (*ADDRESS_GET_CONSTANT(bool, ADDRESS_BOOL_UP_ARROW_STATE))
	{
		selection_index = max(0, selection_index - 1);
	}
	else if (*ADDRESS_GET_CONSTANT(bool, ADDRESS_BOOL_DOWN_ARROW_STATE))
	{
		selection_index = min(selection_index + 1, campaign_names_length - 1);
	}
	else if (*ADDRESS_GET_CONSTANT(bool, ADDRESS_BOOL_ENTER_STATE))
	{
		hook_set_profile_campaign(hook_get_current_profile(), selection_index == 0 ? NULL : scs_get_campaign_directory_by_index(selection_index));

		int* app_state = address_acquire_data(ADDRESS_INT_APP_STATE, sizeof * app_state);
		*app_state = 0;
		address_release_data(app_state);
		return SCS_RESULT_CREATE_STATE;
	}
	else if (*ADDRESS_GET_CONSTANT(bool, ADDRESS_BOOL_ESCAPE_STATE))
	{
		int* app_state = address_acquire_data(ADDRESS_INT_APP_STATE, sizeof * app_state);
		*app_state = 1;
		address_release_data(app_state);
		return SCS_RESULT_CONTINUE;
	}

	int width = max(longest_name_length, sizeof WINDOW_TITLE - 1) + 4;
	int height = max(30, campaign_names_length + 3);
	int x = TARGET_WIDTH / 2 - width / 2;
	int y = TARGET_HEIGHT / 2 - height / 2;

	CHAR_INFO* screen_data = address_acquire_ptr(ADDRESS_PTR_SCREEN_DATA, sizeof * screen_data * TARGET_WIDTH * TARGET_HEIGHT);

	scs_render_rectangle(screen_data, (CHAR_INFO) { .Char.AsciiChar = 0, .Attributes = CREATE_ATTRIBUTE(LIGHT_WHITE, DARK_BLACK) }, x, y, width, height);
	scs_render_border(screen_data, CREATE_ATTRIBUTE(LIGHT_WHITE, DARK_BLACK), x, y, width, height);
	scs_render_text(screen_data, WINDOW_TITLE, x + width / 2 - sizeof WINDOW_TITLE / 2, y + 1);

	scs_render_rectangle(screen_data, (CHAR_INFO) { .Char.AsciiChar = 0, .Attributes = CREATE_ATTRIBUTE(DARK_BLACK, LIGHT_WHITE) }, x + 1, y + 2 + selection_index, width - 2, 1);
	char* curr = campaign_names_array;
	for (int i = 0; i < campaign_names_length; i++)
	{
		curr += scs_render_text(screen_data, curr, x + 1, y + 2 + i) + 1;
	}

	address_release_data(screen_data);

	ADDRESS_CALL_SCREEN_INVALIDATE();
	return SCS_RESULT_CONTINUE;
}