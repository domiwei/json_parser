#include "stdio.h"
#include "stdlib.h"
#include "errno.h"

enum {	
	STRING, INTEGER, JSON
};

typedef struct JSON_MAPPING {
	char *key;
	struct JSON_T *value;
} JSON_MAPPING;

typedef struct KEY_LIST_NODE {
	JSON_MAPPING mapping;
	struct KEY_LIST_NODE *next;
} KEY_LIST_NODE;

typedef struct KEY_STRUCT {
	int num_keys;
	struct KEY_LIST_NODE *head;
	struct KEY_LIST_NODE *tail;
} KEY_STRUCT;

typedef struct JSON_T {
	char type; /* STRING, INTEGER, JSON */
	union {
		char *str;
		long long integer;
		struct LIST *list;
		struct KEY_STRUCT *keys;
	};
} JSON_T;

typedef struct {
	struct JSON_T *data;
	struct LIST *next;
} LIST;

int _parse_symbol(FILE *fptr, char *target, char *first_meet)
{
	char c;
	int ret, i;

	ret = -1;
	while ((c = getc(fptr)) != EOF) {
		for (i = 0; i < strlen(target); i++) {
			//printf("target = %c\n", target[i]);
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
			ret = -1;
			break;
		}
	}

	return ret;
}

int _parse_string(FILE *fptr, char **string)
{
	char c;
	int ret;
	char *sptr;

	sptr = malloc(sizeof(char) * 256);
	*string = sptr;

	ret = -1;
	while ((c = getc(fptr)) != EOF) {
		if (c == '"') {
			ret = 0;
			break;
		} else {
			*sptr++ = c;
		}
	}

	*sptr = 0;
	return ret;
}

int _parse_integer(FILE *fptr, long long *num)
{
	char c;

	*num = 0;
	while ((c = getc(fptr)) != EOF ) {
		if (0 <= c - '0' && c - '0' <= 9 ) {
			*num = *num * 10 + (c - '0');
		} else {
			break;
		}
	}

	fseek(fptr, -1, SEEK_CUR);
	return 0;
}

void _print_indent(long long layer)
{
	int i;

	for (i = 0; i < layer; i++)
		fputs("\t", stdout);
}

void _print_json(JSON_T *json_obj, long long layer)
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

#define _PARSE_SYMBOL(fptr, str_ptr, first_meet_ptr) \
	{ \
		ret = _parse_symbol(fptr, str_ptr, first_meet_ptr); \
		if (ret < 0) { \
			printf("Error in %d\n", __LINE__); \
			return ret; \
		} \
	}

void print_json(JSON_T *json_obj)
{
	_print_json(json_obj, 0);
}

int insert_json_obj(JSON_T *json_obj, char *key, JSON_T *value)
{
	KEY_LIST_NODE *node;
	KEY_STRUCT **keys;

	keys = &(json_obj->keys);
	if ((*keys) == NULL) {
		*keys = malloc(sizeof(KEY_STRUCT));
		memset(*keys, 0, sizeof(KEY_STRUCT));
	}

	node = malloc(sizeof(KEY_LIST_NODE));
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

	if (json_obj->type != JSON)
		return -EINVAL;

	now = json_obj->keys->head;
	while (now) {
		printf("now key is %s\n", now->mapping.key);
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

void free_json_obj(JSON_T *json_obj)
{
	KEY_LIST_NODE *now;

	switch (json_obj->type) {
	case STRING:
		free(json_obj->str);
		break;
	case INTEGER:
		break;
	case JSON:
		now = json_obj->keys->head;
		while (now) {
			free(now->mapping.key);
			free_json_obj(now->mapping.value);
			now = now->next; 
		}
		free(json_obj->keys);
		free(json_obj);
		break;
	default:
		break;
	}

	return;
}

int parse_json_file(FILE *fptr, JSON_T *json_obj)
{
	int ret;
	char *key;
	char first_meet;
	char *tmpstr;
	long long tmpnum;
	JSON_T *value;

	if (!json_obj)
		return -ENOMEM;

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
			return ret;

		printf("string = %s : ", key);

		_PARSE_SYMBOL(fptr, ":", NULL);
		_PARSE_SYMBOL(fptr, "\"{[-0123456789", &first_meet);
		value = malloc(sizeof(JSON_T));
		memset(value, 0, sizeof(JSON_T));

		switch (first_meet) {
		case '\"':
			value->type = STRING;
			ret = _parse_string(fptr, &tmpstr);
			if (ret < 0)
				return ret;
			value->str = tmpstr;
			printf("value = %s\n", tmpstr);
			break;
		case '{':
			fseek(fptr, -1, SEEK_CUR);
			ret = parse_json_file(fptr, value);
			if (ret < 0)
				return ret;
			break;
		case '-':
			value->type = INTEGER;
			ret = _parse_integer(fptr, &tmpnum);
			if (ret < 0)
				return ret;
			value->integer = -tmpnum;
			printf("value = %lld\n", -tmpnum);
			break;
		default:
			fseek(fptr, -1, SEEK_CUR);
			value->type = INTEGER;
			ret = _parse_integer(fptr, &tmpnum);
			if (ret < 0)
				return ret;
			value->integer = tmpnum;
			printf("value = %lld\n", tmpnum);
			break;	
		}

		/* Insert */
		insert_json_obj(json_obj, key, value);

		_PARSE_SYMBOL(fptr, "},", &first_meet);
		if (first_meet == '}')
			break;
	}

	return 0;
}

int main()
{
	FILE *fptr;
	int ret;
	JSON_T *json_obj;

	json_obj = malloc(sizeof(JSON_T));
	memset(json_obj, 0, sizeof(JSON_T));
	
	fptr = fopen("test_file", "r");
	ret = parse_json_file(fptr, json_obj);
	printf("ret = %d\n", ret);
	fclose(fptr);

	printf("num_keys = %d\n", json_obj->keys->num_keys);

	JSON_T *json_value;
	json_value = NULL;
	ret = get_json_val(json_obj, "hadha", &json_value);
	if (ret < 0)
		printf("not found\n");
	else
		printf("value is %d\n", json_value->integer);
	print_json(json_obj);
	free_json_obj(json_obj);
	return 0;
}
