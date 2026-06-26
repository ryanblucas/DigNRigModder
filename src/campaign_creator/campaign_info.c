/*
	campaign_info.c ~ RL
*/

#include "campaign_info.h"

static info_internal_t internal;

void campaign_info_initialize(info_internal_t* _internal)
{
	internal = *_internal;
}

void campaign_info_destroy(void)
{

}

void campaign_info_show(bool is_visible)
{

}

bool campaign_info_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, LRESULT* out)
{
	return false;
}

bool campaign_info_handle_interact_tree_item(bool is_global, element_t element)
{
	return false;
}