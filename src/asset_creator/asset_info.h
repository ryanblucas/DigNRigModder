/*
	asset_info.h ~ RL
*/

#include "../action_buffer.h"
#include "../info_box.h"

void asset_info_initialize(info_internal_t* internal);
void asset_info_destroy(void);

void asset_info_show(bool is_visible);
bool asset_info_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, LRESULT* out);

void asset_info_handle_interact_tree_item(bool is_global, element_t element);

/* The pointer to the asset must remain valid until another call to this function that passes NULL. */
void asset_info_set(asset_t* asset);
void asset_info_set_current(region_t region);

action_buffer_t asset_info_action_buffer_get(void);
void asset_info_action_buffer_set(action_buffer_t action_buffer);

info_tool_t asset_info_get_current_tool(void);
void asset_info_get_current_brush_block(asset_block_t* res);
int asset_info_get_current_brush_size(void);

void asset_info_directory_set(const char* directory);
void asset_info_directory_get(char* directory, size_t buf_size);

void asset_info_palette_save(asset_block_t* palette, size_t palette_size);
void asset_info_palette_copy(const asset_block_t* palette, size_t palette_size);