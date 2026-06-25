/*
	select_campaign_state.h ~ RL
	Select campaign state, started after opening an empty save slot
*/

#pragma once

enum scs_result
{
	SCS_RESULT_CREATE_STATE,
	SCS_RESULT_CONTINUE,
};

void sce_initialize(void);
void __cdecl scs_start(void);
enum scs_result __cdecl scs_update(void);