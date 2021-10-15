#ifndef INITPROC_H
#define INITPROC_H

void deallocate(support_t*  toDeallocate);

support_t* allocate();

void init_supLevSem();

void createProc(int id);

void InstantiatorProcess();

#endif