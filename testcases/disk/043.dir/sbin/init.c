#include "sys.h"
#include "libc.h"

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
  // Tests whether we can write to a file,
  // jump to the beginning, and read can actually see the changes
  int fd = open("write", 0);
  char* src = "*** stdout\n";
  write(fd, src, 11);
  seek(fd, 0);
  char result[12];
  int n = read(fd, result, 11);
  result[n] = 0;
  printf("%s", result);
  shutdown();

  return 0;
}
