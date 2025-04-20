#include <stdbool.h>
#include "sys.h"
#include "libc.h"

void error(const char* msg) {
    printf("%s", msg);
    exit(-1);
}