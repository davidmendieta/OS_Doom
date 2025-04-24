#include <stddef.h>
#include <stdbool.h>

typedef union {
    int idata;
    char* sdata;
} value_t;

typedef enum {
    TYPE_INT,
    TYPE_STRING,
} valuemeta_t;