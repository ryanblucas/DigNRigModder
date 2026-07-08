/*
	campaign_main.h ~ RL
*/

#pragma once

#include "../file.h"

enum campaign_property_id
{
	CPI_ENABLE_END_BOX,
	CPI_DISABLE_END_BOX,
	CPI_ENABLE_SHOW_BINARY_MODE,
	CPI_DISABLE_SHOW_BINARY_MODE,
	CPI_CHANGE_TO_ASSET_MODE,
	CPI_CHANGE_TO_BINARY_MODE,
};
/* needs to be cast from void* to this, so better typedef this instead of the actual enum */
typedef uintptr_t campaign_property_id_t;

typedef enum campaign_mode
{
	CAMPAIGN_MODE_ASSET,
	CAMPAIGN_MODE_BINARY,
} campaign_mode_t;

void campaign_initialize(editor_state_t* state);
void campaign_destroy(void);
void campaign_start(void);
void campaign_end(void);