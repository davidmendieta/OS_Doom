#include "libc.h"
#include "hashmap.h"

uint32_t hash_hash(const char* key) {
    uint32_t hash = 2166136261u;
    while (*key) {
        hash ^= (uint8_t)(*key++);
        hash *= 16777619u;
    }
    return hash;
}

void hash_init(Hashmap* map) {
    for (int i = 0; i < ROWS * COLS; i++) {
        map->table[i].tag = NULL;
    }
}

HashValue* hash_get(Hashmap* map, const char* key) {
    uint32_t hash = hash_hash(key) % ROWS;
    uint32_t start = hash * COLS;

    for (int i = start; i < ROWS * COLS; i++) {
        HashValue* node = &map->table[i];
        if (node->tag && streq(node->tag, key)) {
            return node;
        }
    }

    // second chance for linear probing
    for (int i = 0; i < start; i++) {
        HashValue* node = &map->table[i];
        if (streq(node->tag, key)) {
            return node;
        }
    }

    return NULL;
}

// overwrites existing keys, manages allocated memory on removal
// this means you CANNOT pass const char* types
void hash_put(Hashmap* map, const char* key, ValueType type, Value p) {
    uint32_t hash = hash_hash(key) % ROWS;
    uint32_t start = hash * COLS;

    for (int i = start; i < ROWS * COLS; i++) {
        HashValue* node = &map->table[i];
        if (node->tag == NULL || streq(node->tag, key)) {
            if (node->tag && node->type == TYPE_STRING) {
                free(node->data.sdata);
            }
            node->tag = key;
            node->type = type;
            memcpy(&node->data, &p, sizeof(void*));
            return;
        }
    }

    // second chance for linear probing
    for (int i = 0; i < start; i++) {
        HashValue* node = &map->table[i];
        if (node->tag == NULL || streq(node->tag, key)) {
            if (node->tag && node->type == TYPE_STRING) {
                free(node->data.sdata);
            }
            node->tag = key;
            node->type = type;
            memcpy(&node->data, &p, sizeof(void*));
            return;
        }
    }

    // panic!!!!
}

// removes a key, returns the old value, NULL if none
HashValue* hash_remove(Hashmap* map, const char* key) {
    uint32_t hash = hash_hash(key) % ROWS;
    uint32_t start = hash * COLS;

    for (int i = start; i < ROWS * COLS; i++) {
        HashValue* node = &map->table[i];
        if (node->tag && streq(key, node->tag)) {
            node->tag = NULL;
            return node;
        }
    }

    for (int i = 0; i < start; i++) {
        HashValue* node = &map->table[i];
        if (node->tag && streq(key, node->tag)) {
            node->tag = NULL;
            return node;
        }
    }

    // it's like it was never even there
    return NULL;
}
