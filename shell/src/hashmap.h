#include "value.h"

#define ROWS 67
#define COLS 10

typedef struct {
    valuemeta_t type;
    value_t data;
    const char* tag;
} HashValue;

// Hashmap that implements linear probing and memory management
typedef struct Hashmap {
    HashValue table[ROWS * COLS]; // easier for linear probing
    uint32_t avail;
} Hashmap;

void hash_init(Hashmap* map);
// no hash free because variables are forever

HashValue* hash_get(Hashmap* map, const char* key);
void hash_put(Hashmap* map, const char* key, valuemeta_t type, value_t p);
HashValue* hash_remove(Hashmap* map, const char* key);