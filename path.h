/*
	path.h ~ RL
*/

#pragma once

#include "types.h"

/* Finds path of Dig-N-Rig in program files and appends subdirectory to it if
	not NULL. This path contains the executable and all assets. Returns buf
	if successful, NULL otherwise. If no subdirectory is specified, the result
	will have no trailing backslash. */
char* path_find_dnr_main(char* buf, size_t buf_size, const char* subdirectory);
/* Finds path of Dig-N-Rig in documents and appends subdirectory to it if not NULL.
	This path contains saves and settings. Returns buf if successful, NULL otherwise.
	If no subdirectory is specified, the result will have no trailing backslash*/
char* path_find_dnr_docs(char* buf, size_t buf_size, const char* subdirectory);
/* Finds path of Dig-N-Rig save file. Returns buf. This is equivalent to calling
	path_find_dnr_docs(buf, buf_size, "profile##.sav"), where ## is the save. */
char* path_find_dnr_save(char* buf, size_t buf_size, int save);