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

char* path_enumerate_directory_create(const char* directory, int* max)
{
	WIN32_FIND_DATAA wfd = { 0 };
	HANDLE next = FindFirstFileA(directory, &wfd);
	if (!next)
	{
		return NULL;
	}
	/* sure hope there's not INT_MAX layers someone made! */
	int start = *max > 0 ? *max : 0;
	size_t reserved = 8 * MAX_PATH;
	char* result = dig_malloc(reserved);
	char* curr = result - 1;
	do
	{
		curr++;
		size_t len = strnlen(wfd.cFileName, sizeof wfd.cFileName);
		size_t curr_len = curr - result;
		if (curr_len + len >= reserved)
		{
			char* next = dig_malloc(reserved * 2);
			strncpy(next, result, curr_len);
			free(result);
			result = next;
			curr = result + curr_len;
		}
		strncpy(curr, wfd.cFileName, sizeof wfd.cFileName);
		curr += len;
		*curr = 0;
	} while (--*max != 0 && FindNextFileA(next, &wfd));
	DWORD error = GetLastError();
	/* ERROR_NO_TOKEN refers to if permissions used to run this application don't allow it to get all the metadata--not that important */
	RUNTIME_ASSERT(error == 0 || error == ERROR_NO_MORE_FILES || error == ERROR_NO_TOKEN);
	FindClose(next);
	*max = abs(start - *max);
	return result;
}

const char* path_get_file_name(const char* directory)
{
	const char* name = (const char*)strrchr(directory, '\\');
	if (!name)
	{
		name = directory - 1;
	}
	return name + 1;
}

bool path_exists(const char* directory)
{
	FILE* f = fopen(directory, "r");
	if (!f)
	{
		/* clear errno */
		errno;
		return false;
	}
	return true;
}