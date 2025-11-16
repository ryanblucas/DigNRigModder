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

#define _UNEXPECTED_TOKEN_MESSAGE(file, tok, etype) debug_format("(%i) Unexpected token %i, expected %i at line %i, col %i\n", __LINE__, tok.type, etype, (file)->line, (file)->col);
#define MATCH_AND_ADVANCE_TOKEN(file, tok, etype) if (tok.type != (etype)) { _UNEXPECTED_TOKEN_MESSAGE(file, tok, etype); goto cleanup; } else { file_next(file, &tok); }
#define MATCH_TOKEN(file, tok, etype) if (tok.type != (etype)) { _UNEXPECTED_TOKEN_MESSAGE(file, tok, etype); goto cleanup; }
#define ENSURE_CONDITION(file, cond) if (!(cond)) { debug_format("(%i) Failed condition " #cond " at line %i, col %i\n", __LINE__, (file)->line, (file)->col); goto cleanup; }

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
		"PaletteColor" - Number to identify which color palette the console will render it with.
		"Transparency" - 2-D array of unknown type with size WidthXHeight
		"Z" - Unknown purpose, but both in-game sprites have multiple image and color headings and are for the scientist.
	*/

	int width = 0, height = 0;
	char* text = NULL;
	attribute_t* color = NULL;

	struct token curr;
	file_next(pfile, &curr);
	while (curr.type != TOKEN_EOF)
	{
		MATCH_AND_ADVANCE_TOKEN(pfile, curr, TOKEN_HASHTAG);
		MATCH_TOKEN(pfile, curr, TOKEN_STRING);
		if (strncmp(curr.data.str, "Width", DATA_STRING_MAX_SIZE) == 0)
		{
			MATCH_AND_ADVANCE_TOKEN(pfile, curr, TOKEN_STRING);
			MATCH_AND_ADVANCE_TOKEN(pfile, curr, TOKEN_NEWLINE);

			ENSURE_CONDITION(pfile, curr.type == TOKEN_INTEGER);
			width = curr.data.integer;
			MATCH_AND_ADVANCE_TOKEN(pfile, curr, TOKEN_INTEGER);
		}
		else if (strncmp(curr.data.str, "Height", DATA_STRING_MAX_SIZE) == 0)
		{
			MATCH_AND_ADVANCE_TOKEN(pfile, curr, TOKEN_STRING);
			MATCH_AND_ADVANCE_TOKEN(pfile, curr, TOKEN_NEWLINE);

			ENSURE_CONDITION(pfile, curr.type == TOKEN_INTEGER);
			height = curr.data.integer;
			MATCH_AND_ADVANCE_TOKEN(pfile, curr, TOKEN_INTEGER);
		}
		else if (strncmp(curr.data.str, "Image", DATA_STRING_MAX_SIZE) == 0)
		{
			MATCH_AND_ADVANCE_TOKEN(pfile, curr, TOKEN_STRING);
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
			MATCH_AND_ADVANCE_TOKEN(pfile, curr, TOKEN_STRING);
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
		else if (strncmp(curr.data.str, "TileType", DATA_STRING_MAX_SIZE) == 0
			|| strncmp(curr.data.str, "X weather", DATA_STRING_MAX_SIZE) == 0
			|| strncmp(curr.data.str, "PaletteColor", DATA_STRING_MAX_SIZE) == 0
			|| strncmp(curr.data.str, "Transparency", DATA_STRING_MAX_SIZE) == 0)
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

	fclose(file.handle);
	sprite_t res = screen_sprite_create(width, height, text, color);
	free(text);
	free(color);
	return res;
cleanup:
	free(text);
	free(color);
	fclose(file.handle);
	return NULL;
}