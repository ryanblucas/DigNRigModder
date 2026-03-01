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

sprite_t file_sprite_load(const char* directory)
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
	uint32_t palette_id = DNR_DEFAULT_DIRT_COLOR;
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

dnr_state_t* file_state_load(const char* directory)
{
	FILE* file = fopen(directory, "rb");
	if (!file)
	{
		debug_format("File \"%s\" does not exist\n", directory);
		return NULL;
	}

	dnr_state_t* res = dig_malloc(sizeof * res + sizeof * res->stalactite_array * DEFAULT_STALACTITE_COUNT);
	res->stalactite_count = DEFAULT_STALACTITE_COUNT;
	res->stalactite_array = (stalactite_t*)(res + 1);

	/* read up to stalactites */
	BINARY_ENSURE_CONDITION(fread(res, offsetof(dnr_state_t, stalactite_array), 1, file) == 1);
	for (int i = 0; i < res->stalactite_count; i++)
	{
		res->stalactite_array[i].cell.Char.AsciiChar = 0x1F;
		res->stalactite_array[i].cell.Attributes = 0x06;
		BINARY_ENSURE_CONDITION(fread(&res->stalactite_array[i].exists, 1, 4, file) == 4);
		BINARY_ENSURE_CONDITION(fread(&res->stalactite_array[i].x, 1, 4, file) == 4);
		BINARY_ENSURE_CONDITION(fread(&res->stalactite_array[i].y, 1, 4, file) == 4);
		BINARY_ENSURE_CONDITION(fread(&res->stalactite_array[i].falling, 1, 4, file) == 4);
		BINARY_ENSURE_CONDITION(fread(&res->stalactite_array[i].activation_radius_2, 1, 4, file) == 4);
		BINARY_ENSURE_CONDITION(fread(&res->stalactite_array[i].speed, 1, 4, file) == 4);
	}

	/* read up to end */
	BINARY_ENSURE_CONDITION(fread(res->reserved2, sizeof res->reserved2, 1, file) == 1);
	BINARY_ENSURE_CONDITION(fread(&res->has_liquid_resistance, sizeof res->has_liquid_resistance, 1, file) == 1);

	fclose(file);
	return res;

cleanup:
	fclose(file);
	free(res);
	return NULL;
}

void file_state_unload(dnr_state_t* save)
{
	free(save);
}

bool file_state_save(const char* directory, const dnr_state_t* save)
{
	FILE* file = fopen(directory, "wb");
	if (!file)
	{
		debug_format("Could not open file \"%s\"\n", directory);
		return false;
	}

	bool result = false;
	
	BINARY_ENSURE_CONDITION(fwrite(save, offsetof(dnr_state_t, stalactite_array), 1, file) == 1);
	for (int i = 0; i < save->stalactite_count; i++)
	{
		BINARY_ENSURE_CONDITION(fwrite(&save->stalactite_array[i].exists, 1, 4, file) == 4);
		BINARY_ENSURE_CONDITION(fwrite(&save->stalactite_array[i].x, 1, 4, file) == 4);
		BINARY_ENSURE_CONDITION(fwrite(&save->stalactite_array[i].y, 1, 4, file) == 4);
		BINARY_ENSURE_CONDITION(fwrite(&save->stalactite_array[i].falling, 1, 4, file) == 4);
		BINARY_ENSURE_CONDITION(fwrite(&save->stalactite_array[i].activation_radius_2, 1, 4, file) == 4);
		BINARY_ENSURE_CONDITION(fwrite(&save->stalactite_array[i].speed, 1, 4, file) == 4);
	}

	BINARY_ENSURE_CONDITION(fwrite(save->reserved2, sizeof save->reserved2, 1, file) == 1);
	BINARY_ENSURE_CONDITION(fwrite(&save->has_liquid_resistance, sizeof save->has_liquid_resistance, 1, file) == 1);

	result = true;
cleanup:
	fclose(file);
	return result;
}

CHAR_INFO file_state_spritify_cell(const dnr_state_t* save, int x, int y)
{
	const dnr_block_t* curr = &save->blocks[x * LAYER_COUNT * TARGET_HEIGHT + y];
	CHAR_INFO final = curr->visual;

	if (curr->rig_type == RIG_LAVA)
	{
		final.Attributes = DARK_RED << 4;
	}
	else if (curr->rig_type == RIG_WATER)
	{
		final.Attributes = DARK_BLUE << 4;
	}
	
	if (final.Char.AsciiChar != ' ')
	{
		return final;
	}

	if (curr->mineral_exists)
	{
		RUNTIME_ASSERT(curr->mineral_index >= 0 && curr->mineral_index < sizeof save->minerals / sizeof * save->minerals);
		const dnr_mineral_t* mineral = &save->minerals[curr->mineral_index];
		if (mineral->exists)
		{
			final.Char.AsciiChar = (char)mineral->size;
			final.Attributes = final.Attributes & 0xF0 | (mineral->type & 0x0F);
		}
	}
	for (int i = 0; i < save->stalactite_count; i++)
	{
		if (save->stalactite_array[i].exists && (int)save->stalactite_array[i].x == x && (int)save->stalactite_array[i].y == y)
		{
			final = save->stalactite_array[i].cell;
			break;
		}
	}
	return final;
}

sprite_t file_state_spritify(const dnr_state_t* save, int layer_index)
{
	RUNTIME_ASSERT(save && layer_index >= 0 && layer_index < LAYER_COUNT);
	char* text = dig_malloc(TARGET_WIDTH * TARGET_HEIGHT * sizeof * text);
	attribute_t* attrib = dig_malloc(TARGET_WIDTH * TARGET_HEIGHT * sizeof * attrib);

	for (int x = 0; x < TARGET_WIDTH; x++)
	{
		for (int y = 0; y < TARGET_HEIGHT; y++)
		{
			CHAR_INFO final = file_state_spritify_cell(save, x, y + layer_index * TARGET_HEIGHT);

			text[x + y * TARGET_WIDTH] = final.Char.AsciiChar;
			attrib[x + y * TARGET_WIDTH] = final.Attributes;
		}
	}

	sprite_t res = screen_sprite_create(TARGET_WIDTH, TARGET_HEIGHT, save->layer_headers[layer_index].dirt_color, text, attrib);

	free(text);
	free(attrib);
	return res;
}