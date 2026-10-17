#pragma once

struct sysinfo {
    /* leave empty or put whatever fields you actually need */
};

#ifdef __cplusplus
extern "C" {
#endif
int sysinfo(struct sysinfo *);
#ifdef __cplusplus
}
#endif
