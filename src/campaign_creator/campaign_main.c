/*
	campaign_main.c ~ RL
*/

#include "campaign_main.h"
#include "campaign_info.h"
#include "../info_box.h"

static editor_state_t* editor_state;

void campaign_initialize(editor_state_t* state)
{
	editor_state = state;
	info_mode_class_t class =
	{
		.mode = MODE_CAMPAIGN,
		.caption = "Campaign",
		.initialize = campaign_info_initialize,
		.destroy = campaign_info_destroy,
		.show = campaign_info_show,
		.proc = campaign_info_proc,
		.interact_tree_item = campaign_info_handle_interact_tree_item
	};
	info_add_class(&class);
}

void campaign_destroy(void)
{

}

void campaign_start(void)
{

}

void campaign_end(void)
{

}