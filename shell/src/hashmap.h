#include <stddef.h>

#define ROWS 67
#define COLS 10

typedef enum {
    TYPE_INT,
    TYPE_STRING
} ValueType;

typedef union {
    int32_t idata;
    char* sdata;
} Value;

typedef struct {
    ValueType type;
    Value data;
    const char* tag;
} HashValue;

// Hashmap that implements linear probing
typedef struct {
    HashValue table[ROWS * COLS]; // easier for linear probing
} Hashmap;

void hash_init(Hashmap* map);
// no hash free because variables are forever

HashValue* hash_get(Hashmap* map, const char* key);
void hash_put(Hashmap* map, const char* key, ValueType type, Value p);
void hash_remove(Hashmap* map, const char* key);