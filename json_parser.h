enum {	
	STRING, INTEGER, JSON, FLOAT
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
	char type; /* STRING, INTEGER, JSON, FLOAT */
	union {
		char *str;
		long long integer;
		double floating;
		struct LIST *list;
		struct KEY_STRUCT *keys;
	};
} JSON_T;

typedef struct {
	struct JSON_T *data;
	struct LIST *next;
} LIST;

int parse_json_file(FILE *fptr, JSON_T *json_obj);
int insert_json_obj(JSON_T *json_obj, char *key, JSON_T *value);
int get_json_val(JSON_T *json_obj, char *key, JSON_T **value);
void free_json_obj(JSON_T *json_obj);
void free_json_obj_field(JSON_T *json_obj);
