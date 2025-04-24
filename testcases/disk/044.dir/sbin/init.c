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
  // Test the system call to create a directory
  // and then create a file in that directory
  // and write to it and read from it
  int nd = mkdir("/sbin/newdir");
  printf("mkdir /sbin/newdir: %d\n", nd);
  int fd = open("/sbin/newdir/test.txt", 0x0200, 0644);

  char* src = "*** stdout\n";
  write(fd, src, 11);
  close(fd);
  int fd2= open("/sbin/newdir/test.txt", 0x0200, 0644);
  char result[12];
  int n = read(fd2, result, 11);
  result[n] = 0;
  printf("%s", result);
  close(fd);
  shutdown();
  return 0;
}
