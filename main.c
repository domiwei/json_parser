#include <stdio.h>
#include <stdlib.h>
#include "json_parser.h"

/* Testing */
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
	if (ret < 0)
		return 0;

	printf("num_keys = %d\n", json_obj->keys->num_keys);

	JSON_T *json_value;
	json_value = NULL;
	ret = get_json_val(json_obj, "keweifloat", &json_value);
	if (ret < 0) {
		printf("not found\n");
	} else {
		switch (json_value->type) {
		case STRING:
			printf("string is %s\n", json_value->str);
			break;
		case INTEGER:
			printf("integer is %lld\n", json_value->integer);
			break;
		case FLOAT:
			printf("floating num is %.20lf\n", json_value->floating);
			break;
		case JSON:
		default:
			break;
		}
	}
	//print_json(json_obj);
	free_json_obj(json_obj);
	//free(json_obj);
	return 0;
}
