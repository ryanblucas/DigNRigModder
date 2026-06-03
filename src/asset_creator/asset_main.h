/*
	asset_main.h ~ RL
*/

#include "../file.h"

void asset_initialize(editor_state_t* state);
void asset_destroy(void);
void asset_start(void);
void asset_end(void);

bool asset_can_change_field(const void* field);