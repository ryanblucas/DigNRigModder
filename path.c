/*
	path.c ~ RL
*/

#include "path.h"
#include <stdio.h>
#include <Windows.h>
#include <ShlObj.h>

static char* path_find_internal(char* buf, size_t buf_size, const char* user_subdir, const KNOWNFOLDERID* folder_id)
{
	WCHAR* wbuf;
	if (FAILED(SHGetKnownFolderPath(folder_id, 0, NULL, &wbuf)))
	{
		CoTaskMemFree(wbuf);
		return NULL;
	}
	
	char buf_main[MAX_PATH];
	WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wbuf, -1, buf_main, sizeof buf_main, NULL, NULL);
	CoTaskMemFree(wbuf);
	if (user_subdir)
	{
		snprintf(buf, buf_size, "%s\\DigiPen\\Dig-N-Rig\\%s", buf_main, user_subdir);
	}
	else
	{
		snprintf(buf, buf_size, "%s\\DigiPen\\Dig-N-Rig", buf_main);
	}
	return buf;
}

char* path_find_dnr_main(char* buf, size_t buf_size, const char* subdirectory)
{
	return path_find_internal(buf, buf_size, subdirectory, &FOLDERID_ProgramFilesX86);
}

char* path_find_dnr_docs(char* buf, size_t buf_size, const char* subdirectory)
{
	return path_find_internal(buf, buf_size, subdirectory, &FOLDERID_Documents);
}

char* path_find_dnr_save(char* buf, size_t buf_size, int save)
{
	char subdirectory[20];
	snprintf(subdirectory, sizeof subdirectory, "profile%i.sav", save);
	return path_find_dnr_docs(buf, buf_size, subdirectory);
}