#ifndef INTERRUPTS_H_INCLUDED
#define INTERRUPTS_H_INCLUDED

void interruptHandler(unsigned int cause);
void allInterr(int lineNum);
void genInt(int lineNum, int devNum);
#endif
