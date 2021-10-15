#ifndef VMSUPPORT_H
#define VMSUPPORT_H

#include "../resources/pandos_types.h"

void init_swap();

void TLB_exceptionHandler();

void killProc(int *sem);

void clearSwap(int asid);

void updateTLB(pteEntry_t *updatedEntry);

int findReplace();

void writeReadFlash(unsigned int RoW, unsigned int devNumber, unsigned int occupiedPageNumber, int swap_pool_index);

void dirtyPage(unsigned int currASID, unsigned int occupiedPageNumber, unsigned int swap_pool_index);

pteEntry_t *findEntry(unsigned int pageNumber);

void uTLB_refillHandler();

#endif