#include <umps3/umps/aout.h>
#include <umps3/umps/arch.h>
#include <umps3/umps/libumps.h>
#include "pandos_libs.h"
#include "../resources/pandos_const.h"
#include "../resources/pandos_types.h"
#include "../phase_1/pcb.h"
#include "../phase_1/asl.h"
#include "scheduler.h"
#include "exceptions.h"
#include "interrupts.h"

/*Pointer to the pcbs in "ready" state*/
pcb_PTR readyQueue;

/*SoftBlocked pcbs counter: pcbs that are "blocked" due to an interrupt*/
int softBlockedCount;

/*Total processes counter*/
int processCount;

/* Pointer to the "running" pcb */
pcb_PTR currentProcess;

/*Array of devices semaphores*/
int dSemaphores[MAX_SEMAPHORES];

/*Initial processor state*/
state_t initState;

/*Pass-Up-Vector declaration*/
passupvector_t* PassUpVector;

/*Testing function*/
extern void InstantiatorProcess();

/*TLB-Handler function*/
extern void uTLB_refillHandler();

extern void exceptionHandler();


/*Initial function, called at the start*/
int main() {
    /* INITIALIZING PCBS, ASL AND COUNTERS */
    processCount = 0;
    softBlockedCount = 0;

    readyQueue = mkEmptyProcQ();

    initPcbs();
    initASL();

    /* INITALIZING PASS-UP-VECTOR */
    PassUpVector = (passupvector_t*) PASSUPVECTOR;

    PassUpVector->tlb_refill_handler = (memaddr) uTLB_refillHandler;
    PassUpVector->tlb_refill_stackPtr = (memaddr) KERNELSTACK;
    PassUpVector->exception_handler = (memaddr) exceptionHandler;
    PassUpVector->exception_stackPtr = (memaddr) KERNELSTACK;

    /* INITIALIZING SEMAPHORES */
    for(int i = 0; i < MAX_SEMAPHORES; i++)
        dSemaphores[i] = 0;

    /* LOADING INTERVAL TIMER */
    LDIT(PSECOND);

    /* PROCESS INITIALIZATION */
    pcb_PTR initProc = allocPcb();

    initProc->p_time = 0;
    initProc->p_semAdd = NULL;
    initProc->p_supportStruct = NULL;

    processCount++;

    /* CREATING A STATE AREA */
    STST(&initState);

    /* SETTING SP TO RAMTOP */
    RAMTOP(initState.reg_sp);

    /* INITIALIZING THE INITIAL STATE TO TEST FUNCTION */
    initState.pc_epc = (memaddr) InstantiatorProcess;
    initState.reg_t9 = (memaddr) InstantiatorProcess;

    /* INTERRUPT ENABLED AND PLT ENABLED */
    initState.status = IEPON | IMON | TEBITON;

    /* SAVING THE STATE IN THE FIRST PROCESS */
    initProc->p_s = initState;

    /* SETTING THE FIRST PROCESS AS "READY" */
    currentProcess = NULL;
    insertProcQ(&(readyQueue), initProc);

    scheduler();

    return 0;
}
