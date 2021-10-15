#include "scheduler.h"
#include "../resources/pandos_types.h"
#include "../phase_1/pcb.h"
#include <umps3/umps/libumps.h>

/* Getting the "global" counters and processes */
extern pcb_PTR readyQueue, currentProcess;
extern int softBlockedCount, processCount;

/* Saved time of the currently "running" process */
cpu_t insertTime;

void scheduler(){
    /* Saving the currentProcess "running" time */
    if(currentProcess != NULL)
        currentProcess->p_time += (CURRENT_TOD - insertTime);

    /* Making "run" a "ready" process */
    currentProcess = removeProcQ(&readyQueue);

    /* CASE: there was actually a process waiting */
    if(currentProcess != NULL) {
        /* SAVING THE STARTING TIME */
        STCK(insertTime);
        /* SETTING PLT TIMESLICE */
        setTIMER(TIME_CONVERT(PSECOND));
        /* LOADING THE NEW PROCESS STATE */
        LDST(&(currentProcess->p_s));
    } /* CASE: there was no process waiting */
    else {
        /* CASE: (probably) all fine */
        if(processCount == 0) HALT();
        else if (processCount > 0 && softBlockedCount > 0)   /* CASE: process(es) soft-blocked */
        {
            /* SORT OF DISABLING PLT-TIMER */
            setTIMER(TIME_CONVERT(1000000000));
            /* ENABLING INTERRUPTS */
            setSTATUS(IECON | IMON);
            /* WAIT STATE */
            WAIT();
        }
        else if(processCount > 0 && softBlockedCount == 0) /* CASE: deadlock */
            PANIC();
    }
}