/*
	layer_info.h ~ RL
*/

#include "../info_box.h"

void layer_info_initialize(const info_internal_t* internal);
void layer_info_destroy(void);

void layer_info_show(bool is_visible);
bool layer_info_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, LRESULT* out);