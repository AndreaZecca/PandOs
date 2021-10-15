#include <umps3/umps/libumps.h>
#include <umps3/umps/types.h>
#include <umps3/umps/cp0.h>
#include <umps3/umps/arch.h>
#include "../resources/pandos_const.h"
#include "../resources/pandos_types.h"
#include "pandos_libs.h"
#include "../phase_1/pcb.h"
#include "../phase_1/asl.h"
#include "interrupts.h"
#include "scheduler.h"
#include "exceptions.h"

/* Getting "global" counters, processes and the insertTime */
extern pcb_PTR readyQueue, currentProcess;
extern int softBlockedCount, processCount;
extern cpu_t insertTime;

unsigned int exceptionCode;
unsigned int sysCallCode;

/* Getting devices semaphores */
extern int dSemaphores[MAX_SEMAPHORES];

/* Pre-exception processor state */
state_t* exceptState;

void SyscallExceptionHandler(state_t* exception_state){
    sysCallCode = (unsigned int) exception_state->reg_a0;
    exceptState = exception_state;
    exceptState->pc_epc += WORDLEN;
    if((exception_state->status & USERPON) != ALLOFF){
        //SYSCALL CALLED IN USER MODE
        unsigned int oldCause= getCAUSE() & !CAUSE_EXCCODE_MASK;
        setCAUSE(oldCause | 40);
        PassUp(GENERALEXCEPT, exception_state);
        
        /*exception_state->cause |= (10<<CAUSESHIFT);
        PassUp(GENERALEXCEPT, exception_state);*/
    }
    

    switch (sysCallCode)
    {
        case CREATEPROCESS:
            Create_Process_SYS1();
            break;
        case TERMPROCESS:
            Terminate_Process_SYS2();
            break;
        case PASSEREN:
            Passeren_SYS3((int*) exception_state->reg_a1);
            break;
        case VERHOGEN:
            Verhogen_SYS4((int*) exception_state->reg_a1);
            break;
        case IOWAIT:
            Wait_For_IO_Device_SYS5();
            break;
        case GETTIME:
            Get_CPU_Time_SYS6();
            break;
        case CLOCKWAIT:
            Wait_For_Clock_SYS7();
            break;
        case GETSUPPORTPTR:
            Get_SUPPORT_Data_SYS8();
            break;
        default:
            PassUp(GENERALEXCEPT, exception_state);
            break;
    }
}

void Create_Process_SYS1(){
    //ALLOCATING NEW PCB
    pcb_PTR newProc = allocPcb();
    /* CASE: i can allocate a new process */
    if(newProc != NULL) {
        /* ADD A NEW PROCESS TO THE COUNTER */
        processCount++;

        /* SETTING THE GIVEN SEMAPHORE AND SUPPORT STRUCT */
        newProc->p_s = *((state_t *) exceptState->reg_a1);
        newProc->p_supportStruct = (support_t *) exceptState->reg_a2;

        /* INSERTING THE NEW PROCESS IN THE PROCESSES TREE AND SETTING ITS STATUS ON "READY" */
        insertChild(currentProcess, newProc);
        insertProcQ(&(readyQueue), newProc);

        /* SUCCESSFUL SYSCALL */
        exceptState->reg_v0 = OK;
    } else {
        /* UNSUCCESSFUL SYSCALL */
       exceptState->reg_v0 = -1;
    }
    /* RELOADING OLD STATE */
    LDST(exceptState);
}

void Terminate_Process_SYS2(){
    outChild(currentProcess);
    removeProgeny(currentProcess);
    currentProcess = NULL;
    scheduler();
}

HIDDEN void removeProgeny(pcb_t* toRemove){
    if (toRemove == NULL) return;

    while (!(emptyChild(toRemove)))
        removeProgeny(removeChild(toRemove));

    removeSingleProcess(toRemove);
}

HIDDEN void removeSingleProcess(pcb_t* toRemove){
    processCount -= 1;
    outProcQ(&readyQueue, toRemove);

    if (toRemove->p_semAdd != NULL)
    {
        //if inutile non capiamo il perchè
        if (&(dSemaphores[0]) <= toRemove->p_semAdd && toRemove->p_semAdd <= &(dSemaphores[48])){
            softBlockedCount -= 1;
        }
        else{
            *(toRemove->p_semAdd) += 1;
        }
        outBlocked(toRemove);
    }
    freePcb(toRemove);
}

void Passeren_SYS3(int* semAddr){
    /* DECREASING SEMAPHORE VALUE */
    (*semAddr)--;
    /* CASE: The process has to be blocked */
    if(*semAddr < 0){
        /* SAVING ON THE PROCESS THE OLD PROCESSOR STATE */
        currentProcess->p_s = *exceptState;
        /* BLOCKING THE PROCESS ON THE SEMAPHORE */
        insertBlocked(semAddr, currentProcess);
        /* CALLING scheduler */
        scheduler();
    } else {
        /* PROCESS NOT BLOCKED */
        LDST(exceptState);
    }
}

void Verhogen_SYS4(int* semAddr){
    /* INCREASING SEMAPHORE VALUE */
    (*semAddr)++;

    /* UNBLOCKING A PREVIOUSLY BLOCKED PROCESS */
    pcb_PTR  unblockedProcess = removeBlocked(semAddr);

    /* CASE: I have actually unblocked a process */
    if(unblockedProcess != NULL){
        /* PROCESS HAS NO SEMAPHORE ANYMORE */
        unblockedProcess->p_semAdd = NULL;

        /* SETTING PROCESS STATUS ON "READY" */
        insertProcQ(&readyQueue, unblockedProcess);
    }
    /* LOADING OLD PROCESSOR STATE */
    LDST(exceptState);
}

void Wait_For_IO_Device_SYS5(){    
    /* GETTING INFORMATION ABOUT LINE AND DEVICE */
    int lineNum = exceptState->reg_a1;
    int devNum = exceptState->reg_a2;
    /* TERMINAL INTERRUPT ON RECEIVING OR TRANSMISSION */
    int RorT = exceptState->reg_a3;
    /*SEMAPHORE INDEX*/
    int semaphoreIndex = DEV_INDEX(lineNum, devNum, RorT);
    int *sem = &dSemaphores[semaphoreIndex];
    (*sem)--;
    /*PROCESS IS SOFTBLOCKED ON THE SEMAPHORE*/
    insertBlocked(sem, currentProcess);
    softBlockedCount++; 
    currentProcess->p_s = *exceptState;
    /*CALL THE SCHEDULER*/
    scheduler();    
}

void Get_CPU_Time_SYS6(){
    /* ADDING THE CURRENTLY STORED TIME */
    currentProcess->p_time += (CURRENT_TOD - insertTime);
    /* STORING IT IN v0 REGISTER */
    exceptState->reg_v0 = currentProcess->p_time;
    /* SETTING A NEW INSERTION TIME */
    STCK(insertTime);
    /* LOADING OLD PROCESSOR STATE */
    LDST(exceptState);
}

void Wait_For_Clock_SYS7(){
    /* A NEW PROCESS IS NOW BLOCKED DUE TO IT */
    softBlockedCount++;
    /* PASSEREN ON THE INTERVAL TIMER SEMAPHORE */
    Passeren_SYS3(&(dSemaphores[MAX_SEMAPHORES-1]));
}

void Get_SUPPORT_Data_SYS8(){
  /* LOADING SUPPORT DATA ON v0 REGISTER */
  exceptState->reg_v0 = (unsigned int) currentProcess->p_supportStruct;
  /* LOADING OLD PROCESSOR STATE */
  LDST(exceptState);
}

void exceptionHandler(){
    state_t *exceptionState = (state_t*)BIOSDATAPAGE;
    exceptionCode = CAUSE_GET_EXCCODE(exceptionState->cause);
    if (exceptionCode == 0)
        interruptHandler(exceptionState->cause);
    else if ((exceptionCode >= 1) && (exceptionCode <= 3))
        PassUp(PGFAULTEXCEPT, exceptionState);
    else if (exceptionCode == 8)
        SyscallExceptionHandler(exceptionState);
    else
        PassUp(GENERALEXCEPT, exceptionState);
}

void PassUp(int except_type, state_t* exceptionState){
    if(currentProcess->p_supportStruct == NULL){
        Terminate_Process_SYS2();
    }
    else{
        (currentProcess->p_supportStruct)->sup_exceptState[except_type] = *exceptionState;
        context_t info_to_pass = (currentProcess->p_supportStruct)->sup_exceptContext[except_type];
        LDCXT(info_to_pass.c_stackPtr, info_to_pass.c_status, info_to_pass.c_pc);
    }
}
