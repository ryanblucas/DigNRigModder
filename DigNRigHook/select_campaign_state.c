/*
	select_campaign_state.c ~ RL
	Select campaign state, started after opening an empty save slot
*/

#include "select_campaign_state.h"
#include "address.h"

void __cdecl scs_start(void)
{
	int* app_state = address_acquire_data(ADDRESS_INT_APP_STATE, sizeof * app_state);
	*app_state = 4;
	address_release_data(app_state);
}

int __cdecl scs_update(void)
{
	/* to do: menu music stops playing, JMP to the code that runs the music in
       main_menu_render and put a code cave after to JE back if state == 4 */
	/* to do: animate dig-n-rig logo in the back */

	if (*ADDRESS_GET_CONSTANT(bool, ADDRESS_BOOL_UP_ARROW_STATE))
	{
		int* app_state = address_acquire_data(ADDRESS_INT_APP_STATE, sizeof * app_state);
		*app_state = 0;
		address_release_data(app_state);
		return 0;
	}
	else if (*ADDRESS_GET_CONSTANT(bool, ADDRESS_BOOL_DOWN_ARROW_STATE))
	{
		int* app_state = address_acquire_data(ADDRESS_INT_APP_STATE, sizeof * app_state);
		*app_state = 1;
		address_release_data(app_state);
	}

	return 1;
}