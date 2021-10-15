#include "pandos_libs.h"

void bpmem(){}

void memcpy(void *dest, const void *src, size_t n)
{
    for (size_t i = 0; i < n; i++)
    {
        ((char*)dest)[i] = ((char*)src)[i];
    }
    bpmem();
}

unsigned int getBits(unsigned int full, int mask, int shift)
{
    full = full & mask;
    full >>= shift;
    return full;
}
