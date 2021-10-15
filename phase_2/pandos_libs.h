#ifndef PANDOS_LIBS_H_INCLUDED
#define PANDOS_LIBS_H_INCLUDED
#include "../resources/pandos_types.h"
#include "../resources/pandos_const.h"
#include "umps3/umps/libumps.h"

typedef __SIZE_TYPE__ size_t;

void memcpy(void *dest, const void *src, size_t n);

unsigned int getBits(unsigned int full, int mask, int shift);

#endif
