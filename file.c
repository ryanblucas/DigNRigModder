/*
	file.c ~ RL
*/

#include "file.h"
#include "debug.h"
#include <math.h>
#include "screen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DATA_STRING_MAX_SIZE 16

#define _UNEXPECTED_TOKEN_MESSAGE(file, tok, etype) debug_format("(%s, %i) Unexpected token %i, expected %i at line %i, col %i\n", directory, __LINE__, tok.type, etype, (file)->line, (file)->col);
#define MATCH_AND_ADVANCE_TOKEN(file, tok, etype) if (tok.type != (etype)) { _UNEXPECTED_TOKEN_MESSAGE(file, tok, etype); goto cleanup; } else { file_next(file, &tok); }
#define MATCH_TOKEN(file, tok, etype) if (tok.type != (etype)) { _UNEXPECTED_TOKEN_MESSAGE(file, tok, etype); goto cleanup; }
#define ENSURE_CONDITION(file, cond) if (!(cond)) { debug_format("(%i) Failed condition " #cond " at line %i, col %i\n", __LINE__, (file)->line, (file)->col); goto cleanup; }
#define BINARY_ENSURE_CONDITION(cond) if (!(cond)) { debug_format("(%i) Failed condition " #cond " in binary file.\n", __LINE__); goto cleanup; }

enum token_type
{
	TOKEN_HASHTAG,
	TOKEN_NEWLINE,
	TOKEN_EOF,
	TOKEN_STRING,
	TOKEN_INTEGER,
	TOKEN_DECIMAL
};

struct file
{
	FILE* handle;
	int line;
	int col;
};

struct token
{
	enum token_type type;
	union
	{
		char str[DATA_STRING_MAX_SIZE];
		int integer;
		float decimal;
	} data;
};

static inline int file_fpeek(struct file* file)
{
	int ch = fgetc(file->handle);
	ungetc(ch, file->handle);
	return ch;
}

static inline int file_fgetc(struct file* file)
{
	int ch = fgetc(file->handle);
	file->col++;
	if (ch == '\n')
	{
		file->line++;
		file->col = 0;
	}
	return ch;
}

static bool file_next(struct file* file, struct token* out)
{
	int ch = file_fpeek(file);
	memset(out, 0, sizeof * out);
	while (ch == ' ')
	{
		ch = file_fgetc(file);
	}

	if (ch == '#')
	{
		out->type = TOKEN_HASHTAG;
		ch = file_fgetc(file);
	}
	else if (ch == '\n')
	{
		out->type = TOKEN_NEWLINE;
		ch = file_fgetc(file);
	}
	else if (ch == EOF)
	{
		out->type = TOKEN_EOF;
		return false;
	}
	else if (ch >= '0' && ch <= '9')
	{
		out->type = TOKEN_INTEGER;
		int num = 0;
		ch = file_fgetc(file);
		while (ch >= '0' && ch <= '9')
		{
			num = num * 10 + ch - '0';
			ch = file_fgetc(file);
		}
		out->data.integer = num;
		if (ch != '.')
		{
			return true;
		}
		ch = file_fgetc(file);
		int dec = 0, len = 0;
		out->type = TOKEN_DECIMAL;
		while (ch >= '0' && ch <= '9')
		{
			dec = dec * 10 + ch - '0';
			len++;
			ch = file_fgetc(file);
		}
		out->data.decimal = (float)num + powf(10, -(float)len) * dec;
	}
	else if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'))
	{
		out->type = TOKEN_STRING;
		int i;
		for (i = 0; file_fpeek(file) != '\n' && file_fpeek(file) != EOF && i < DATA_STRING_MAX_SIZE - 1; i++)
		{
			ch = file_fgetc(file);
			out->data.str[i] = ch;
		}
		if (i >= DATA_STRING_MAX_SIZE)
		{
			debug_format("String \"%s\" read hits max size\n", out->data.str[i]);
		}
	}
	else
	{
		debug_format("Error reading file, unexpected character '%c' or 0x%.02X\n", ch, ch);
		return false;
	}

	return true;
}

static void file_serialize_and_print_token(struct token* token)
{
	switch (token->type)
	{
	case TOKEN_HASHTAG:
		debug_format("Hashtag\n");
		break;
	case TOKEN_NEWLINE:
		debug_format("Newline\n");
		break;
	case TOKEN_EOF:
		debug_format("EOF\n");
		break;
	case TOKEN_STRING:
		debug_format("String \"%s\"\n", token->data.str);
		break;
	case TOKEN_INTEGER:
		debug_format("Integer %i\n", token->data.integer);
		break;
	case TOKEN_DECIMAL:
		debug_format("Decimal %f\n", token->data.decimal);
		break;
	}
}

sprite_t file_load_sprite(const char* directory)
{
	struct file file;
	struct file* pfile = &file;
	file.line = file.col = 0;
	file.handle = fopen(directory, "r");
	if (!file.handle)
	{
		debug_format("File \"%s\" does not exist\n", directory);
		return NULL;
	}

	/*
		"Width" - one number
		"Height" - one number
		"Image" - 2-D array of characters with size WidthXHeight
		"Color" - 2-D array of attributes with size WidthXHeight
		"TileType" - 2-D array of unknown type with size WidthXHeight
		"X weather" - Two integers and one decimal number, determining what weather effects to show
		"PaletteColor" - RGB value (0xBBGGRR) to determine color of dirt
		"Transparency" - 2-D array of unknown type with size WidthXHeight
		"Z" - Just means the file is in "editor format"
	*/

	int width = 0, height = 0;
	char* text = NULL;
	uint32_t palette_id = DEFAULT_DIRT_COLOR;
	attribute_t* color = NULL;
	sprite_t res = NULL;

	struct token curr;
	file_next(pfile, &curr);
	while (curr.type != TOKEN_EOF)
	{
		MATCH_AND_ADVANCE_TOKEN(pfile, curr, TOKEN_HASHTAG);
		MATCH_TOKEN(pfile, curr, TOKEN_STRING);
		if (strncmp(curr.data.str, "Width", DATA_STRING_MAX_SIZE) == 0)
		{
			file_next(pfile, &curr);
			MATCH_AND_ADVANCE_TOKEN(pfile, curr, TOKEN_NEWLINE);

			MATCH_TOKEN(pfile, curr, TOKEN_INTEGER);
			width = curr.data.integer;
			MATCH_AND_ADVANCE_TOKEN(pfile, curr, TOKEN_INTEGER);
		}
		else if (strncmp(curr.data.str, "Height", DATA_STRING_MAX_SIZE) == 0)
		{
			file_next(pfile, &curr);
			MATCH_AND_ADVANCE_TOKEN(pfile, curr, TOKEN_NEWLINE);

			MATCH_TOKEN(pfile, curr, TOKEN_INTEGER);
			height = curr.data.integer;
			MATCH_AND_ADVANCE_TOKEN(pfile, curr, TOKEN_INTEGER);
		}
		else if (strncmp(curr.data.str, "Image", DATA_STRING_MAX_SIZE) == 0)
		{
			file_next(pfile, &curr);
			MATCH_AND_ADVANCE_TOKEN(pfile, curr, TOKEN_NEWLINE);

			ENSURE_CONDITION(pfile, width != 0 && height != 0);

			text = dig_malloc(width * height * sizeof * text);
			char* curr_text = text;

			for (int y = 0; y < height; y++)
			{
				MATCH_TOKEN(pfile, curr, TOKEN_INTEGER);
				for (int x = 0; x < width; x++)
				{
					ENSURE_CONDITION(pfile, (curr.data.integer & 0xFFFFFF00) == 0);
					*curr_text++ = curr.data.integer;
					MATCH_AND_ADVANCE_TOKEN(pfile, curr, TOKEN_INTEGER);
				}
				MATCH_AND_ADVANCE_TOKEN(pfile, curr, TOKEN_NEWLINE);
			}
		}
		else if (strncmp(curr.data.str, "Color", DATA_STRING_MAX_SIZE) == 0)
		{
			file_next(pfile, &curr);
			MATCH_AND_ADVANCE_TOKEN(pfile, curr, TOKEN_NEWLINE);

			ENSURE_CONDITION(pfile, width != 0 && height != 0);

			color = dig_malloc(width * height * sizeof * color);
			attribute_t* curr_color = color;

			for (int y = 0; y < height; y++)
			{
				MATCH_TOKEN(pfile, curr, TOKEN_INTEGER);
				for (int x = 0; x < width; x++)
				{
					ENSURE_CONDITION(pfile, (curr.data.integer & 0xFFFF0000) == 0);
					*curr_color++ = curr.data.integer;
					MATCH_AND_ADVANCE_TOKEN(pfile, curr, TOKEN_INTEGER);
				}
				MATCH_AND_ADVANCE_TOKEN(pfile, curr, TOKEN_NEWLINE);
			}
		}
		else if (strncmp(curr.data.str, "PaletteColor", DATA_STRING_MAX_SIZE) == 0)
		{
			file_next(pfile, &curr);
			MATCH_AND_ADVANCE_TOKEN(pfile, curr, TOKEN_NEWLINE);
			palette_id = curr.data.integer;
			MATCH_AND_ADVANCE_TOKEN(pfile, curr, TOKEN_INTEGER);
		}
		else if (strncmp(curr.data.str, "TileType", DATA_STRING_MAX_SIZE) == 0
			|| strncmp(curr.data.str, "X weather", DATA_STRING_MAX_SIZE) == 0
			|| strncmp(curr.data.str, "Transparency", DATA_STRING_MAX_SIZE) == 0
			|| strncmp(curr.data.str, "Z", DATA_STRING_MAX_SIZE) == 0)
		{
			/* skip to next header */
			while (file_next(pfile, &curr) && curr.type != TOKEN_HASHTAG);
		}
		else
		{
			debug_format("Invalid sprite header \"%s\"\n", curr.data.str);
			goto cleanup;
		}
	}

	ENSURE_CONDITION(pfile, text);
	ENSURE_CONDITION(pfile, color);

	res = screen_sprite_create(width, height, palette_id, text, color);
cleanup:
	free(text);
	free(color);
	fclose(file.handle);
	return res;
}

/* to do: put this out of the block loop. It makes it way faster without all the file seeking */

static int file_find_mineral(FILE* file, int index)
{
	if (index == 0xFFFFFFFF)
	{
		return -1;
	}

	long start = ftell(file);
	int result = -1;

	fseek(file, 0x010D2D58 + index * 0x34, SEEK_SET);
	BINARY_ENSURE_CONDITION(!feof(file));

	float x, y;
	BINARY_ENSURE_CONDITION(fread(&x, 1, 4, file) == 4);
	BINARY_ENSURE_CONDITION(fread(&y, 1, 4, file) == 4);
	short size, type;
	BINARY_ENSURE_CONDITION(fread(&size, 1, 2, file) == 2);
	BINARY_ENSURE_CONDITION(fread(&type, 1, 2, file) == 2);
	int exists;
	BINARY_ENSURE_CONDITION(fread(&exists, 1, 4, file) == 4);
	if (!exists)
	{
		int x = index / (TARGET_HEIGHT * 14);
		int y = index % (TARGET_HEIGHT * 14);
		debug_format("Mineral at location (%i, %i) is marked as non-existent, yet the block at this location directs to it and says it exists.\n", x, y);
	}
	fseek(file, start, SEEK_SET);
	result = type | (size << 16);
cleanup:
	return result;
}

static bool file_parse_blocks(FILE* file, sprite_t image_res[14], uint32_t palettes[14])
{
	/* game stores its blocks vertically. As in, instead of 0x0-TARGET_WIDTH as the first
	row of blocks, its 0x0-(TARGET_HEIGHT*14) is the first column of blocks */

	char* char_curr = dig_malloc(TARGET_WIDTH * TARGET_HEIGHT * sizeof * char_curr * 14);
	attribute_t* attrib_curr = dig_malloc(TARGET_WIDTH * TARGET_HEIGHT * sizeof * attrib_curr * 14);
	char* temp_char = dig_malloc(TARGET_WIDTH * TARGET_HEIGHT);
	attribute_t* temp_attrib = dig_malloc(TARGET_WIDTH * TARGET_HEIGHT * 2);

	bool success = false;

	fseek(file, 0x0318, SEEK_SET);
	BINARY_ENSURE_CONDITION(!feof(file));

	for (int i = 0; i < TARGET_WIDTH * TARGET_HEIGHT * 14; i++)
	{
		fseek(file, 0x18, SEEK_CUR);
		BINARY_ENSURE_CONDITION(fread(char_curr + i, 1, 1, file) == 1);
		fseek(file, 0x01, SEEK_CUR);
		BINARY_ENSURE_CONDITION(fread(attrib_curr + i, 1, 2, file) == 2);
		fseek(file, 0x8, SEEK_CUR);
		int mineral, has_mineral;
		BINARY_ENSURE_CONDITION(fread(&mineral, 1, 4, file) == 4);
		fseek(file, 0x8, SEEK_CUR);
		BINARY_ENSURE_CONDITION(fread(&has_mineral, 1, 4, file) == 4);
		fseek(file, 0x20, SEEK_CUR);

		if (has_mineral)
		{
			int code = file_find_mineral(file, mineral);
			short type = code & 0xFFFF,
				size = (code >> 16) & 0xFFFF;
			*(char_curr + i) = size;
			*(attrib_curr + i) = *(attrib_curr + i) & 0xF0 | (type & 0x0F);
		}
	}

	for (int i = 0; i < 14; i++)
	{
		for (int x = 0; x < TARGET_WIDTH; x++)
		{
			for (int y = 0; y < TARGET_HEIGHT; y++)
			{
				*(temp_char + y * TARGET_WIDTH + x) = *(char_curr + y + x * TARGET_HEIGHT * 14 + i * TARGET_HEIGHT);
				*(temp_attrib + y * TARGET_WIDTH + x) = *(attrib_curr + y + x * TARGET_HEIGHT * 14 + i * TARGET_HEIGHT);
			}
		}
		image_res[i] = screen_sprite_create(TARGET_WIDTH, TARGET_HEIGHT, palettes[i], temp_char, temp_attrib);
	}

	success = true;
cleanup:
	free(temp_char);
	free(temp_attrib);
	free(char_curr);
	free(attrib_curr);
	return success;
}

save_t* file_load_save(const char* directory)
{
	FILE* file = fopen(directory, "rb");
	if (!file)
	{
		debug_format("Save directory \"%s\" does not exist.\n", directory);
		return NULL;
	}

	sprite_t image_res[14] = { 0 };
	uint32_t palettes[14] = { 0 };
	float x_spawn, y_spawn;

	fseek(file, 0x3C, SEEK_SET);
	BINARY_ENSURE_CONDITION(!feof(file));

	for (int i = 0; i < 14; i++)
	{
		fseek(file, 0x14, SEEK_CUR);
		BINARY_ENSURE_CONDITION(fread(&palettes[i], 1, 4, file) == 4);
	}

	fseek(file, 0x01FC, SEEK_SET);
	BINARY_ENSURE_CONDITION(!feof(file));

	BINARY_ENSURE_CONDITION(fread(&x_spawn, 1, sizeof x_spawn, file) == sizeof x_spawn);
	BINARY_ENSURE_CONDITION(fread(&y_spawn, 1, sizeof y_spawn, file) == sizeof y_spawn);

	BINARY_ENSURE_CONDITION(file_parse_blocks(file, image_res, palettes));

	save_t* res = dig_malloc(sizeof * res + sizeof * res->layer_images * 14);
	memcpy(res->layer_images, image_res, sizeof image_res);
	res->layer_count = 14;
	res->x_spawn = x_spawn;
	res->y_spawn = y_spawn;

	return res;
cleanup:
	for (int i = 0; i < sizeof image_res / sizeof * image_res; i++)
	{
		free(image_res[i]);
	}
	fclose(file);
	return NULL;
}

void file_unload_save(save_t* save)
{
	if (!save)
	{
		return;
	}
	for (int i = 0; i < save->layer_count; i++)
	{
		screen_sprite_destroy(save->layer_images[i]);
	}
	free(save);
}