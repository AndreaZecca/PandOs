#ifndef EXCEPTIONS_H_INCLUDED
#define EXCEPTIONS_H_INCLUDED

void PassUp(int except_type, state_t* exceptionState);

void Create_Process_SYS1();

void Terminate_Process_SYS2();

HIDDEN void removeSingleProcess(pcb_t* toRemove);

HIDDEN void removeProgeny(pcb_t* toRemove);

void Passeren_SYS3(int* semAddr);

void Verhogen_SYS4(int* semAddr);

void Wait_For_IO_Device_SYS5();

void Get_CPU_Time_SYS6();

void Wait_For_Clock_SYS7();

void Get_SUPPORT_Data_SYS8();

void exceptionHandler();
#endif
