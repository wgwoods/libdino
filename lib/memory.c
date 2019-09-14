#include <unistd.h>
#include "memory.h"

size_t PAGESIZE = 4096;
size_t CHONKSIZE = 4096 << 10;

void __attribute__ ((constructor)) _memory_init_pagesize(void) {
    long sys_pagesize = sysconf(_SC_PAGESIZE);
    if (sys_pagesize > 0) PAGESIZE = sys_pagesize;
    long sys_chonksize = sysconf(_SC_LEVEL3_CACHE_SIZE);
    if (sys_chonksize > 0) CHONKSIZE = sys_chonksize;
}

