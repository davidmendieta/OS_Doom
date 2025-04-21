#include "sys.h"

// https://gcc.gnu.org/onlinedocs/gcc/Statement-Exprs.html
#define log(s)                                                                 \
  ({                                                                           \
    int i = -1;                                                                \
    while (s[++i] != 0) {                                                      \
      int rc = write(1, &s[i], 1);                                             \
      if (rc != 1)                                                             \
        break;                                                                 \
    }                                                                          \
  })

int main(int argc, char **argv) {
  log("*** :3\n");
  shutdown();
  return 0;
}

// This does nothing.
//
// It's just so I don't have to change a line in `crt0.S`.
//
// Though, this is kinda interesting...
// Why would the C runtime need this?
void heap_init(void) {
  // ...
}
