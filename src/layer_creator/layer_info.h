/*
	layer_info.h ~ RL
*/

#include "../action_buffer.h"
#include "../info_box.h"

void layer_info_initialize(info_internal_t* internal);
void layer_info_destroy(void);

void layer_info_show(bool is_visible);
bool layer_info_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, LRESULT* out);

void layer_info_handle_interact_tree_item(bool is_global, element_t element);

/* The pointer to the asset must remain valid until another call to this function that passes NULL. */
void layer_info_asset_set(asset_t* asset);
void layer_info_asset_set_current(region_t region);

action_buffer_t layer_info_action_buffer_get(void);
void layer_info_action_buffer_set(action_buffer_t action_buffer);

info_tool_t layer_info_get_current_tool(void);
void layer_info_get_current_brush_block(asset_block_t* res);
int layer_info_get_current_brush_size(void);

void layer_info_directory_set(const char* directory);
void layer_info_directory_get(char* directory, size_t buf_size);