#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "json_parser.h"

#define ALLOC_UNIT 256

#define _PARSE_SYMBOL(fptr, str_ptr, first_meet_ptr) \
	{\
		ret = _parse_symbol(fptr, str_ptr, first_meet_ptr); \
		if (ret < 0) {\
			printf("Error in file pos %ld, code line %d\n", ftell(fptr), __LINE__); \
				goto error_handle;\
		}\
	}

static int _parse_symbol(FILE *fptr, char *target, char *first_meet)
{
	char c;
	int ret, i;

	ret = -EINVAL;
	while ((c = getc(fptr)) != EOF) {
		for (i = 0; i < strlen(target); i++) {
			if (c == target[i]) {
				if (first_meet)
					*first_meet = c;
				ret = 0;
				break;
			}
		}
		if (ret == 0)
			break;

		if (c == '\t' || c == ' ' || c == '\n' || c == '\r') {
			continue;
		} else {
			printf("encounter %c\n", c);
			ret = -EINVAL;
			break;
		}
	}

	return ret;
}

static int _parse_string(FILE *fptr, char **string)
{
	char c;
	int ret;
	char *sptr;
	long long mem_size, now_len;

	sptr = malloc(sizeof(char) * ALLOC_UNIT);
	if (!sptr)
		return -ENOMEM;
	mem_size = ALLOC_UNIT;
	*string = sptr;
	now_len = 0;

	ret = -EINVAL;
	while ((c = getc(fptr)) != EOF) {
		if (c == '"') {
			ret = 0;
			break;
		} else {
			*sptr++ = c;
			now_len++;
			if (now_len == mem_size) {
				sptr = realloc(*string, mem_size + ALLOC_UNIT);
				if (!sptr)
					return -ENOMEM;
				mem_size += ALLOC_UNIT;
				*string = sptr;
				sptr += now_len;
			}
		}
	}

	*sptr = 0;
	return ret;
}

static int _parse_integer(FILE *fptr, long long *num)
{
	char c;

	*num = 0;
	while ((c = getc(fptr)) != EOF ) {
		if (0 <= c - '0' && c - '0' <= 9 ) {
			*num = *num * 10 + (c - '0');
		} else {
			if (c == ',' || c == ' ' || c == '\n' || c == '\r' || c == '\t')
				break;
			else {
				if (c == '.')
					return 1;
				printf("Error in file pos %ld. Encounter %c\n", ftell(fptr), c);
				return -EINVAL;
			}
		}
	}

	fseek(fptr, -1, SEEK_CUR);
	return 0;
}

static int _parse_decimal(FILE *fptr, double *decimal)
{
	char c;
	double devide_base;

	devide_base = 1.0;
	*decimal = 0.0;
	while ((c = getc(fptr)) != EOF ) {
		if (0 <= c - '0' && c - '0' <= 9 ) {
			*decimal = *decimal * 10 + (c - '0');
			devide_base *= 10;
		} else {
			if (c == ',' || c == ' ' || c == '\n' || c == '\r' || c == '\t')
				break;
			else {
				printf("Error in file pos %ld. Encounter %c\n", ftell(fptr), c);
				return -EINVAL;
			}
		}
	}
	*decimal /= devide_base;
	fseek(fptr, -1, SEEK_CUR);
	return 0;
}

static inline void _print_indent(long long layer)
{
	int i;

	for (i = 0; i < layer; i++)
		fputs("\t", stdout);
}

static void _print_json(JSON_T *json_obj, long long layer)
{
	KEY_LIST_NODE *now;

	switch (json_obj->type) {
	case STRING:
		fputs("\"", stdout);
		fputs(json_obj->str, stdout);
		fputs("\"", stdout);
		break;
	case INTEGER:
		printf("%lld", json_obj->integer);
		break;
	case JSON:
		fputs("{\n", stdout);
		now = json_obj->keys->head;
		while (now) {
			_print_indent(layer + 1);
			fputs("\"", stdout);
			fputs(now->mapping.key, stdout);
			fputs("\": ", stdout);
			_print_json(now->mapping.value, layer + 1);
			fputs(",\n", stdout);
			now = now->next;
		}
		_print_indent(layer);
		fputs("}", stdout);
	}
}

void print_json(JSON_T *json_obj)
{
	_print_json(json_obj, 0);
}

int insert_json_obj(JSON_T *json_obj, char *key, JSON_T *value)
{
	KEY_LIST_NODE *node;
	KEY_STRUCT **keys;

	if (!json_obj)
		return -ENOMEM;

	if (json_obj->type != JSON)
		return -EINVAL;

	keys = &(json_obj->keys);
	if ((*keys) == NULL) {
		*keys = malloc(sizeof(KEY_STRUCT));
		if (!(*keys))
			return -ENOMEM;
		memset(*keys, 0, sizeof(KEY_STRUCT));
	}

	node = malloc(sizeof(KEY_LIST_NODE));
	if (!node)
		return -ENOMEM;
	node->mapping.key = key;
	node->mapping.value = value;
	node->next = 0;

	if ((*keys)->num_keys > 0) {
		(*keys)->tail->next = node;
		(*keys)->tail = node; 
		(*keys)->num_keys++;
	} else {
		(*keys)->tail = node;
		(*keys)->head = node;
		(*keys)->num_keys = 1;
	}

	return 0;
}

int get_json_val(JSON_T *json_obj, char *key, JSON_T **value)
{
	KEY_LIST_NODE *now;

	if (!json_obj)
		return -ENOENT;

	if (json_obj->type != JSON)
		return -EINVAL;

	if (!(json_obj->keys))
		return -ENOENT;

	now = json_obj->keys->head;
	while (now) {
		if (!strcmp(now->mapping.key, key)) {
			*value = now->mapping.value;
			break;
		}
		now = now->next;
	}

	if (now)
		return 0;
	else
		return -ENOENT;
}

/* Recursively free all key-value structure, including of itself */
void free_json_obj(JSON_T *json_obj)
{
	KEY_LIST_NODE *now, *prev;

	if (!json_obj)
		return;

	switch (json_obj->type) {
	case STRING:
		if (json_obj->str)
			free(json_obj->str);
		free(json_obj);
		break;
	case INTEGER:
		break;
	case JSON:
		if (!(json_obj->keys))
			break;
		now = json_obj->keys->head;
		while (now) {
			free(now->mapping.key);
			free_json_obj(now->mapping.value);
			prev = now;
			now = now->next;
			free(prev);
		}
		free(json_obj->keys);
		free(json_obj);
		break;
	default:
		break;
	}

	return;
}

/* Recursively free all key-value structure, excluding of itself */
void free_json_obj_field(JSON_T *json_obj)
{
	KEY_LIST_NODE *now, *prev;

	if (!json_obj)
		return;

	if (json_obj->type == STRING) {
		free(json_obj->str);
		return;
	} else if (json_obj->type == INTEGER) {
		return;
	}

	if (!(json_obj->keys))
		return;

	now = json_obj->keys->head;
	while (now) {
		free(now->mapping.key);
		free_json_obj(now->mapping.value);
		prev = now;
		now = now->next;
		free(prev);
	}
	free(json_obj->keys);

	return;
}

int parse_json_file(FILE *fptr, JSON_T *json_obj)
{
	int ret;
	char *key;
	char first_meet;
	char *tmpstr;
	long long tmpnum;
	double floatnum, decimal;
	JSON_T *value;

	if (!json_obj)
		return -ENOMEM;

	key = NULL;
	tmpstr = NULL;
	value = NULL;
	memset(json_obj, 0, sizeof(JSON_T));
	json_obj->type = JSON;
	json_obj->keys = NULL;

	_PARSE_SYMBOL(fptr, "{", NULL);

	while (!feof(fptr)) {
		_PARSE_SYMBOL(fptr, "\"}", &first_meet);
		if (first_meet == '}')
			break;

		ret = _parse_string(fptr, &key);
		if (ret < 0)
			goto error_handle;
		//printf("string = %s : ", key);

		_PARSE_SYMBOL(fptr, ":", NULL);
		_PARSE_SYMBOL(fptr, "\"{-0123456789.", &first_meet);
		value = malloc(sizeof(JSON_T));
		if (!value) {
			ret = -ENOMEM;
			goto error_handle;
		}
		memset(value, 0, sizeof(JSON_T));

		switch (first_meet) {
		case '\"':
			ret = _parse_string(fptr, &tmpstr);
			if (ret < 0)
				goto error_handle;
			value->type = STRING;
			value->str = tmpstr;
			//printf("value = %s\n", tmpstr);
			break;
		case '{':
			fseek(fptr, -1, SEEK_CUR);
			ret = parse_json_file(fptr, value);
			if (ret < 0)
				goto error_handle;
			break;
		case '-':
			ret = _parse_integer(fptr, &tmpnum);
			if (ret < 0) {
				goto error_handle;
			} else if (ret == 1) {
				ret = _parse_decimal(fptr, &decimal);
				if (ret < 0)
					goto error_handle;
				floatnum = tmpnum + decimal;
				value->type = FLOAT;
				value->floating = -floatnum;
			} else {
				value->type = INTEGER;
				value->integer = -tmpnum;
			}
			//printf("value = %lld\n", -tmpnum);
			break;
		default:
			fseek(fptr, -1, SEEK_CUR);
			ret = _parse_integer(fptr, &tmpnum);
			if (ret < 0) {
				goto error_handle;
			} else if (ret == 1) {
				ret = _parse_decimal(fptr, &decimal);
				if (ret < 0)
					goto error_handle;
				floatnum = tmpnum + decimal;
				value->type = FLOAT;
				value->floating = floatnum;
			} else {
				value->type = INTEGER;
				value->integer = tmpnum;
			}
			//printf("value = %lld\n", tmpnum);
			break;	
		}

		/* Insert */
		ret = insert_json_obj(json_obj, key, value);
		if (ret < 0)
			goto error_handle;

		key = NULL;
		tmpstr = NULL;
		value = NULL;

		_PARSE_SYMBOL(fptr, "},", &first_meet);
		if (first_meet == '}')
			break;
	}

	return 0;

error_handle:
	/* Do not free self because it perhaps results in
	double free error when recursing */
	free_json_obj_field(json_obj);
	if (key)
		free(key);
	if (tmpstr)
		free(tmpstr);
	if (value)
		free(value);
	return ret;
}
