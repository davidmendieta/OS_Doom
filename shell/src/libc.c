#include "libc.h"

int putchar(int c) {
    char t = (char)c;
    return write(1,&t,1);
}

int puts(const char* p) {
    char c;
    int count = 0;
    while ((c = *p++) != 0) {
        int n = putchar(c); 
        if (n < 0) return n;
        count ++;
    }
    putchar('\n');
    
    return count+1;
}

int my_atoi(const char *str) {
    int result = 0;
    int sign = 1;

    // Check for sign
    if (*str == '-' || *str == '+') {
        if (*str == '-') {
            sign = -1;
        }
        str++;
    }

    // Convert digits
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }

    return sign * result;
}

void cp(int from, int to) {
    while (1) {
        char buf[100];
        ssize_t n = read(from,buf,100);
        if (n == 0) break;
        if (n < 0) {
            printf("*** %s:%d read error, fd = %d\n",__FILE__,__LINE__,from);
            break;
        }
        char *ptr = buf;
        while (n > 0) {
            ssize_t m = write(to,ptr,n);
            if (m < 0) {
                printf("*** %s:%d write error, fd = %d\n",__FILE__,__LINE__,to);
                break;
            }
            n -= m;
            ptr += m;
        }
    }
}

int streq(const char* s1, const char* s2) {
    while (*s1 && *s2) {
        if (*s1 != *s2) return 0;
        s1++;
        s2++;
    }
    return *s1 == *s2;
}

int strlen(const char *str) {
    int index = 0;
    while (str[index]) {
        index++;
    }
    return index;
}

char* strcat(char* dest, const char* src) {
    char* ptr = dest;
    while (*ptr != '\0') {
        ptr++;
    }
    while (*src != '\0') {
        *ptr = *src;
        ptr++;
        src++;
    }
    *ptr = '\0';
    return dest;
}

char* strcpy(char* dest, const char* src) {
    char* ptr = dest;
    while (*src != '\0') {
        *ptr++ = *src++;
    }
    *ptr = '\0';
    return dest;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    while (n--) {
        if (*s1 != *s2) {
            return *(unsigned char *)s1 - *(unsigned char *)s2;
        }
        if (*s1 == '\0') {
            return 0;
        }
        s1++;
        s2++;
    }
    return 0;
}    

char* strchr(const char* s, int ch) {
    while (*s) {
        if (*s == (char)ch) {
            return (char*)s;
        }
        s++;
    }
    if (ch == '\0') {
        return (char*)s;
    }
    return 0;
}
