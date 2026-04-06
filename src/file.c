/*
	file.c ~ RL
*/

#include "file.h"
#include "debug.h"
#include "game.h"
#include <math.h>
#include "path.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DATA_STRING_MAX_SIZE 272

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

static inline int file_fprintf(struct file* file, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int res = vfprintf(file->handle, fmt, args);
	va_end(args);

	/* This doesn't work if a %s is passed in that has multiple lines. For the purposes of this module, it doesn't matter */
	while (*fmt)
	{
		if (*fmt == '\n')
		{
			file->line++;
			file->col = 0;
		}
		else
		{
			file->col++;
		}
		fmt++;
	}

	return res;
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

bool file_editor_load(editor_state_t* state)
{
	struct file file = { 0 };
	char directory[MAX_PATH];
	file.handle = fopen(path_find_dnr_docs(directory, sizeof directory, "editor_config.ini"), "r");
	if (!file.handle)
	{
		return false;
	}

	memset(state, 0, sizeof * state);

	bool result = false;
	struct token curr;
	file_next(&file, &curr);
	while (curr.type != TOKEN_EOF)
	{
		MATCH_AND_ADVANCE_TOKEN(&file, curr, TOKEN_HASHTAG);
		MATCH_TOKEN(&file, curr, TOKEN_STRING);
		if (strncmp(curr.data.str, "CurrentSave", DATA_STRING_MAX_SIZE) == 0)
		{
			file_next(&file, &curr);
			MATCH_AND_ADVANCE_TOKEN(&file, curr, TOKEN_NEWLINE);
			MATCH_TOKEN(&file, curr, TOKEN_INTEGER);
			state->current_save = curr.data.integer;
			file_next(&file, &curr);
		}
		else if (strncmp(curr.data.str, "CurrentMode", DATA_STRING_MAX_SIZE) == 0)
		{
			file_next(&file, &curr);
			MATCH_AND_ADVANCE_TOKEN(&file, curr, TOKEN_NEWLINE);
			MATCH_TOKEN(&file, curr, TOKEN_STRING);
			if (strncmp(curr.data.str, "Save", DATA_STRING_MAX_SIZE) == 0)
			{
				state->current_mode = MODE_SAVE;
			}
			else if (strncmp(curr.data.str, "Layer", DATA_STRING_MAX_SIZE) == 0)
			{
				state->current_mode = MODE_LAYER;
			}
			file_next(&file, &curr);
		}
		else if (strncmp(curr.data.str, "CurrentLayerDirectory", DATA_STRING_MAX_SIZE) == 0)
		{
			file_next(&file, &curr);
			MATCH_AND_ADVANCE_TOKEN(&file, curr, TOKEN_NEWLINE);
			MATCH_TOKEN(&file, curr, TOKEN_STRING);
			snprintf(state->current_layer_directory, MAX_PATH, "%s", curr.data.str);
			file_next(&file, &curr);
		}
		else
		{
			debug_format("Invalid editor config header \"%s\"\n", curr.data.str);
			goto cleanup;
		}
		while (curr.type == TOKEN_NEWLINE)
		{
			file_next(&file, &curr);
		}
	}

	result = true;
cleanup:
	fclose(file.handle);
	return result;
}

bool file_editor_save(const editor_state_t* state)
{
	char directory[MAX_PATH];
	FILE* file = fopen(path_find_dnr_docs(directory, sizeof directory, "editor_config.ini"), "w");
	if (!file)
	{
		debug_format("Failed to open editor config file for writing\n");
		return false;
	}

	fprintf(file, "#CurrentSave\n%i\n", state->current_save);

	const char* mode_descriptor = NULL;
	switch (state->current_mode)
	{
	case MODE_SAVE:
		mode_descriptor = "Save";
		break;
	case MODE_LAYER:
		mode_descriptor = "Layer";
		break;
	}
	fprintf(file, "#CurrentMode\n%s\n", mode_descriptor);
	fprintf(file, "#CurrentLayerDirectory\n%s\n", state->current_layer_directory);
	
	fclose(file);
	return true;
}

static bool file_asset_parse_attribute_single(const char* directory, struct file* file, struct token* pcurr, int* out)
{
	struct token curr = *pcurr;
	bool result = false;
	file_next(file, &curr);
	MATCH_AND_ADVANCE_TOKEN(file, curr, TOKEN_NEWLINE);
	*out = curr.data.integer;
	MATCH_AND_ADVANCE_TOKEN(file, curr, TOKEN_INTEGER);
	result = true;
cleanup:
	*pcurr = curr;
	return result;
}

static bool file_asset_parse_attribute_array(const char* directory, struct file* file, struct token* pcurr, asset_t* res, size_t offset)
{
	struct token curr = *pcurr;
	bool result = false;
	file_next(file, &curr);
	MATCH_AND_ADVANCE_TOKEN(file, curr, TOKEN_NEWLINE);
	ENSURE_CONDITION(file, res->width != 0 && res->height != 0 && res->blocks);
	for (int y = 0; y < res->height; y++)
	{
		MATCH_TOKEN(file, curr, TOKEN_INTEGER);
		for (int x = 0; x < res->width; x++)
		{
			*(int*)((uint8_t*)(res->blocks + y * res->width + x) + offset) = curr.data.integer;
			MATCH_AND_ADVANCE_TOKEN(file, curr, TOKEN_INTEGER);
		}
		MATCH_AND_ADVANCE_TOKEN(file, curr, TOKEN_NEWLINE);
	}
	result = true;
cleanup:
	*pcurr = curr;
	return result;
}

static inline void file_asset_layer_color_correct(asset_t* res)
{
	for (int i = 0; i < res->width * res->height; i++)
	{
		uint8_t ch = (uint8_t)res->blocks[i].visual.Char.AsciiChar;
		if (ch == GAME_STONE_CHAR)
		{
			res->blocks[i].visual.Attributes = CREATE_ATTRIBUTE(DARK_YELLOW, DARK_BLACK);
		}
		else if (ch == 0)
		{
			WORD attrib = res->blocks[i].visual.Attributes;
			res->blocks[i].visual.Attributes = CREATE_ATTRIBUTE(ATTRIBUTE_BACKGROUND(attrib), ATTRIBUTE_BACKGROUND(attrib));
		}
	}
}

asset_t file_asset_load(const char* directory)
{
	asset_t res = { .blocks = NULL, .dirt_color = DNR_DEFAULT_DIRT_COLOR };
	struct file file;
	struct file* pfile = &file;
	file.line = file.col = 0;
	file.handle = fopen(directory, "r");
	if (!file.handle)
	{
		debug_format("File \"%s\" does not exist\n", directory);
		return res;
	}

	bool is_layer = false;
	char* end = strrchr(directory, '.');
	if (end && strncmp(end, ".layer", 6) == 0)
	{
		is_layer = true;
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

	struct token curr;
	file_next(pfile, &curr);
	while (curr.type != TOKEN_EOF)
	{
		MATCH_AND_ADVANCE_TOKEN(pfile, curr, TOKEN_HASHTAG);
		MATCH_TOKEN(pfile, curr, TOKEN_STRING);
		if (strncmp(curr.data.str, "Width", DATA_STRING_MAX_SIZE) == 0)
		{
			ENSURE_CONDITION(pfile, file_asset_parse_attribute_single(directory, pfile, &curr, (int*)&res.width));
		}
		else if (strncmp(curr.data.str, "Height", DATA_STRING_MAX_SIZE) == 0)
		{
			ENSURE_CONDITION(pfile, file_asset_parse_attribute_single(directory, pfile, &curr, (int*)&res.height));
			res.blocks = dig_malloc(res.width * res.height * sizeof * res.blocks);
			memset(res.blocks, 0, res.width * res.height * sizeof * res.blocks);
		}
		else if (strncmp(curr.data.str, "Image", DATA_STRING_MAX_SIZE) == 0)
		{
			ENSURE_CONDITION(pfile, file_asset_parse_attribute_array(directory, pfile, &curr, &res, offsetof(asset_block_t, visual.Char.AsciiChar)));
		}
		else if (strncmp(curr.data.str, "Color", DATA_STRING_MAX_SIZE) == 0)
		{
			ENSURE_CONDITION(pfile, file_asset_parse_attribute_array(directory, pfile, &curr, &res, offsetof(asset_block_t, visual.Attributes)));
			if (is_layer)
			{
				file_asset_layer_color_correct(&res);
			}
		}
		else if (strncmp(curr.data.str, "TileType", DATA_STRING_MAX_SIZE) == 0)
		{
			ENSURE_CONDITION(pfile, file_asset_parse_attribute_array(directory, pfile, &curr, &res, offsetof(asset_block_t, tile_type)));
		}
		else if (strncmp(curr.data.str, "Transparency", DATA_STRING_MAX_SIZE) == 0)
		{
			ENSURE_CONDITION(pfile, file_asset_parse_attribute_array(directory, pfile, &curr, &res, offsetof(asset_block_t, transparency)));
		}
		else if (strncmp(curr.data.str, "PaletteColor", DATA_STRING_MAX_SIZE) == 0)
		{
			ENSURE_CONDITION(pfile, file_asset_parse_attribute_single(directory, pfile, &curr, (int*)&res.dirt_color));
		}
		else if (strncmp(curr.data.str, "X weather", DATA_STRING_MAX_SIZE) == 0)
		{
			ENSURE_CONDITION(pfile, file_asset_parse_attribute_single(directory, pfile, &curr, (int*)&res.weather1));
			MATCH_TOKEN(pfile, curr, TOKEN_INTEGER);
			res.weather2 = curr.data.integer;
			file_next(pfile, &curr);
			MATCH_TOKEN(pfile, curr, TOKEN_DECIMAL);
			res.weather3 = curr.data.decimal;
			/* skip the token just processed, then skip the newline */
			file_next(pfile, &curr);
			file_next(pfile, &curr);
		}
		else if (strncmp(curr.data.str, "Z", DATA_STRING_MAX_SIZE) == 0)
		{
			/* skip to next header */
			while (file_next(pfile, &curr) && curr.type != TOKEN_HASHTAG);
		}
		else
		{
			debug_format("Invalid asset header \"%s\"\n", curr.data.str);
			goto cleanup;
		}
	}

cleanup:
	fclose(file.handle);
	return res;
}

void file_asset_unload(asset_t* asset)
{
	free(asset->blocks);
	*asset = (asset_t){ 0 };
}

bool file_asset_save(const char* directory, const asset_t* asset)
{
	bool result = false;
	struct file mfile = { .handle = fopen(directory, "w"), .line = 0, .col = 0 };
	if (!mfile.handle)
	{
		debug_format("Failed to open file \"%s\" for writing\n", directory);
		return false;
	}
	struct file* file = &mfile;
	ENSURE_CONDITION(file, file_fprintf(file, "#Width\n%i\n#Height\n%i\n", asset->width, asset->height) > 0);
	ENSURE_CONDITION(file, file_fprintf(file, "#Image\n") > 0);
	for (int y = 0; y < asset->height; y++)
	{
		for (int x = 0; x < asset->width; x++)
		{
			ENSURE_CONDITION(file, file_fprintf(file, "%hhu ", asset->blocks[y * asset->width + x].visual.Char.AsciiChar) > 0);
		}
		ENSURE_CONDITION(file, file_fprintf(file, "\n") > 0);
	}
	ENSURE_CONDITION(file, file_fprintf(file, "#Color\n") > 0);
	for (int y = 0; y < asset->height; y++)
	{
		for (int x = 0; x < asset->width; x++)
		{
			ENSURE_CONDITION(file, file_fprintf(file, "%hhu ", asset->blocks[y * asset->width + x].visual.Attributes) > 0);
		}
		ENSURE_CONDITION(file, file_fprintf(file, "\n") > 0);
	}
	ENSURE_CONDITION(file, file_fprintf(file, "#TileType\n") > 0);
	for (int y = 0; y < asset->height; y++)
	{
		for (int x = 0; x < asset->width; x++)
		{
			ENSURE_CONDITION(file, file_fprintf(file, "%hhu ", asset->blocks[y * asset->width + x].tile_type) > 0);
		}
		ENSURE_CONDITION(file, file_fprintf(file, "\n") > 0);
	}
	ENSURE_CONDITION(file, file_fprintf(file, "#Transparency\n") > 0);
	for (int y = 0; y < asset->height; y++)
	{
		for (int x = 0; x < asset->width; x++)
		{
			ENSURE_CONDITION(file, file_fprintf(file, "%hhu ", asset->blocks[y * asset->width + x].transparency) > 0);
		}
		ENSURE_CONDITION(file, file_fprintf(file, "\n") > 0);
	}
	ENSURE_CONDITION(file, file_fprintf(file, "#PaletteColor\n%u\n", asset->dirt_color) > 0);
	ENSURE_CONDITION(file, file_fprintf(file, "#X weather\n%i %i %f\n", asset->weather1, asset->weather2, asset->weather3) > 0);

	result = true;
cleanup:
	fclose(file->handle);
	return result;
}

static void file_state_load_shop_item(const uint32_t* arena, shop_item_t* item, int index)
{
	item->discovered = arena[index];
	item->discovery_percentage = arena[index + 0x1E];
	item->count_max = arena[index + 0x3C];
	item->count_next = arena[index + 0x5A];
	item->count_curr = arena[index + 0x78];
	item->mineral_cost[0] = arena[index * 0x8 + 0x96];
	item->mineral_cost[1] = arena[index * 0x8 + 0x97];
	item->mineral_cost[2] = arena[index * 0x8 + 0x98];
	item->mineral_cost[3] = arena[index * 0x8 + 0x99];
	item->mineral_cost[4] = arena[index * 0x8 + 0x9A];
	item->mineral_cost[5] = arena[index * 0x8 + 0x9B];
	item->mineral_cost[6] = arena[index * 0x8 + 0x9C];
	item->mineral_cost[7] = arena[index * 0x8 + 0x9D];
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
	long curr = ftell(file);
	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, curr, SEEK_SET);
	BINARY_ENSURE_CONDITION(fread((uint8_t*)res + offsetof(dnr_state_t, stalactite_count) + sizeof res->stalactite_count, size - curr, 1, file) == 1);

	file_state_load_shop_item(res->reserved2, &res->dirt_digger, 0);
	file_state_load_shop_item(res->reserved2, &res->rock_drill, 1);
	file_state_load_shop_item(res->reserved2, &res->stone_grinder, 2);
	file_state_load_shop_item(res->reserved2, &res->jump_upgrade, 3);
	file_state_load_shop_item(res->reserved2, &res->jetpack_upgrade, 4);
	file_state_load_shop_item(res->reserved2, &res->elements_resistance, 5);
	file_state_load_shop_item(res->reserved2, &res->scan_upgrade, 6);
	file_state_load_shop_item(res->reserved2, &res->vacpak_upgrade, 7);
	file_state_load_shop_item(res->reserved2, &res->wifi_upgrade, 8);
	file_state_load_shop_item(res->reserved2, &res->health_upgrade, 9);
	file_state_load_shop_item(res->reserved2, &res->battery_upgrade, 10);
	file_state_load_shop_item(res->reserved2, &res->dynamite, 11);
	file_state_load_shop_item(res->reserved2, &res->double_dynamite, 12);
	file_state_load_shop_item(res->reserved2, &res->mega_bomb, 13);
	file_state_load_shop_item(res->reserved2, &res->dirtzooka, 14);
	file_state_load_shop_item(res->reserved2, &res->dirtzooka_upgrade, 15);

	if (!game_is_valid(res))
	{
		debug_format("Read invalid save file\n");
	}

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

static void file_state_save_shop_item(uint32_t* arena, const shop_item_t* item, int index)
{
	arena[index] = item->discovered;
	arena[index + 0x1E] = item->discovery_percentage;
	arena[index + 0x3C] = item->count_max;
	arena[index + 0x5A] = item->count_next;
	arena[index + 0x78] = item->count_curr;
	arena[index * 0x8 + 0x96] = item->mineral_cost[0];
	arena[index * 0x8 + 0x97] = item->mineral_cost[1];
	arena[index * 0x8 + 0x98] = item->mineral_cost[2];
	arena[index * 0x8 + 0x99] = item->mineral_cost[3];
	arena[index * 0x8 + 0x9A] = item->mineral_cost[4];
	arena[index * 0x8 + 0x9B] = item->mineral_cost[5];
	arena[index * 0x8 + 0x9C] = item->mineral_cost[6];
	arena[index * 0x8 + 0x9D] = item->mineral_cost[7];
}

bool file_state_save(const char* directory, const dnr_state_t* save)
{
	if (!game_is_valid(save))
	{
		debug_format("Writing invalid save file!\n");
	}

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

	uint32_t* arena = dig_malloc(sizeof save->reserved2);
	memcpy(arena, save->reserved2, sizeof save->reserved2);
	file_state_save_shop_item(arena, &save->dirt_digger, 0);
	file_state_save_shop_item(arena, &save->rock_drill, 1);
	file_state_save_shop_item(arena, &save->stone_grinder, 2);
	file_state_save_shop_item(arena, &save->jump_upgrade, 3);
	file_state_save_shop_item(arena, &save->jetpack_upgrade, 4);
	file_state_save_shop_item(arena, &save->elements_resistance, 5);
	file_state_save_shop_item(arena, &save->scan_upgrade, 6);
	file_state_save_shop_item(arena, &save->vacpak_upgrade, 7);
	file_state_save_shop_item(arena, &save->wifi_upgrade, 8);
	file_state_save_shop_item(arena, &save->health_upgrade, 9);
	file_state_save_shop_item(arena, &save->battery_upgrade, 10);
	file_state_save_shop_item(arena, &save->dynamite, 11);
	file_state_save_shop_item(arena, &save->double_dynamite, 12);
	file_state_save_shop_item(arena, &save->mega_bomb, 13);
	file_state_save_shop_item(arena, &save->dirtzooka, 14);
	file_state_save_shop_item(arena, &save->dirtzooka_upgrade, 15);
	free(arena);

	size_t offset = offsetof(dnr_state_t, stalactite_count) + sizeof save->stalactite_count;
	BINARY_ENSURE_CONDITION(fwrite((uint8_t*)save + offset, offsetof(dnr_state_t, dirt_digger) - offset, 1, file) == 1);

	result = true;
cleanup:
	fclose(file);
	return result;
}