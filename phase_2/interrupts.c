#include "../resources/pandos_types.h"
#include "../resources/pandos_const.h"
#include "pandos_libs.h"
#include "../phase_1/asl.h"
#include "../phase_1/pcb.h"
#include "scheduler.h"
#include "interrupts.h"
#include <umps3/umps/arch.h>

/* Getting the "global" counters, processes and the insertTime */
extern pcb_PTR readyQueue, currentProcess;
extern int softBlockedCount;
extern cpu_t insertTime;

/* Getting devices semaphores */
extern int dSemaphores[MAX_SEMAPHORES];

/* Interrupt starting time */
cpu_t interrTime;

/* MANAGING ALL INTERRUPTS */
void interruptHandler(unsigned int cause) {
    /* SAVING INTERRUPT STARTING TIME */
    STCK(interrTime);
    /* GET CAUSE LINE */
    unsigned int reg = getBits(cause, 0xFF00, 8);
    int checkLine = 2;
    /* FINDING THE LINE PERFORMING THE INTERRUPT */
    for (int i = 1; i < 8; i++) {
        if (reg & checkLine)
            allInterr(i);
        checkLine *= 2;
    }
}

/* NOW I CAN MANAGE THE (lineNum) INTERRUPT */
void allInterr(int lineNum){
    if(lineNum > 2){
            /*DEVICE INTERRUPT */
            memaddr* currDev = (memaddr*) (CDEV_BITMAP_BASE + ((lineNum - 3) * 0x4));
            int checkDev = 1;
            /* SEARCHING ACTUAL DEVICE */
            for(int i= 0; i < DEVPERINT; i++){
                if(*currDev & checkDev)
                    genInt(lineNum, i);
                checkDev *= 2;
            }
    }
    /* PLT TIMER INTERRUPT */
    else if( lineNum == 1){
        setTIMER(TIME_CONVERT(__INT32_MAX__));
        /* SETTING OLD STATE ON CURRENT PROCESS */
        currentProcess->p_s = *((state_t*) BIOSDATAPAGE);
        /* INCREASING THE PROCESS TIME */
        currentProcess->p_time += (CURRENT_TOD - insertTime);
        /* SETTING PROCESS STATE ON "READY"*/
        insertProcQ(&readyQueue, currentProcess);
        scheduler();
    }
     /* IT INTERRUPT */
    else{
        /* RELOADING INTERVAL TIMER */
        LDIT(PSECOND);
        /* UNBLOCKING ALL PSEUDO-CLOCK SEMPAHORE PROCESSES */
        while(headBlocked(&(dSemaphores[MAX_SEMAPHORES - 1])) != NULL){
            /* UNBLOCKING A PROCESS */
            pcb_PTR unblockedProcess = removeBlocked(&dSemaphores[MAX_SEMAPHORES - 1]);
            /* CASE = there is an actual unblocked process */
            if(unblockedProcess != NULL){
                /* PROCESS NO LONGER BLOCKED ON A SEMAPHORE */
                unblockedProcess->p_semAdd = NULL;
                /* INCREASING PROCESS TIME WITH THE INTERRUPT TIME */
                unblockedProcess->p_time += (CURRENT_TOD - interrTime);
                /* SETTING PROCESS STATUS TO "READY" */
                insertProcQ(&readyQueue, unblockedProcess);
                /* DECREASING NUMBER OF SOFT-BLOCKED PROCESSES */
                softBlockedCount--;
            }
        }
        /* PSEUDOCLOCK SEMAPHORE IS NOW EMPTY */
        dSemaphores[MAX_SEMAPHORES - 1] = 0;
        /* CASE = there is no current process */
        if(currentProcess == NULL)
            scheduler();
        else{
            /* LOADING OLD STATE */
            LDST((state_t*) BIOSDATAPAGE);
        }

    }
}

void genInt(int lineNum, int devNum){
    /* SAVING DEVICE REGISTER */
    devreg_t* devreg = (devreg_t*) (0x10000054 + ((lineNum - 3) * 0x80) + (devNum * 0x10));
    //devreg_t *devreg = (devreg_t*) DEV_REG_ADDR(lineNum, devNum);
    /* TERMINAL RECEIVE OR TRANSMISSION INTERRUPT */
    int RorT = 0;
    /* DECLARING STATUS TO RETURN TO v0 REGISTER */
    unsigned int retStatus;
    //controllo forse sbagliato
    if(lineNum > 2 && lineNum < 7 ){
        /* GIVING INTERRUPT ACKNOWLEDGMENT */
        devreg->dtp.command = READY;
        /* SAVING RETURN STATUS */
        retStatus = devreg->dtp.status;
    }
    else if (lineNum == 7){
        /* CASTING TO TERMINAL REGISTER */
        termreg_t* termreg = (termreg_t*) devreg;
        /* CHECKING WHICH STATUS IS "OK" */
        if(termreg->recv_status != READY){
            /* SAVING RETURN STATUS */
            retStatus = termreg->recv_status;
            /* GIVING INTERRUPT ACKNOWLEDGMENT*/
            termreg->recv_command = ACK;
            /* SETTING RECEIVE INTERRUPT */
            RorT = 1;
        }
        else{
            /* SAVING RETURN STATUS */
            retStatus = termreg->transm_status;
            /* GIVING INTERRUPT ACKNOWLEDGMENT*/
            termreg->transm_command = ACK;
        }
        /* TERMINAL DEVNUM */
        devNum = 2*devNum + RorT;
    }
    /* FINDING DEVICE SEMAPHORE ADDRESS */
    int semAdd = (lineNum - 3) * 8 + devNum;
    /* INCREASING DEVICE SEMAPHORE VALUE */
    dSemaphores[semAdd]++;
    /* UNBLOCKING PROCESS ON THE SEMAPHORE */
    pcb_PTR unblockedProcess = removeBlocked(&dSemaphores[semAdd]);

    /* CHECKING IF THERE WAS ACTUALLY A BLOCKED PROCESS */
    if(unblockedProcess !=  NULL){
        /* RETURNING ASKED STATUS TO v0 REGISTER */
        unblockedProcess->p_s.reg_v0 = retStatus;
        /* PROCESS NO LONGER BLOCKED ON A SEMAPHORE */
        unblockedProcess->p_semAdd = NULL;
        /* INCREASING PROCESS TIME WITH INTERRUPT TIME */
        unblockedProcess->p_time += (CURRENT_TOD - interrTime);
        /* DECREASING NUMBER OF SOFT-BLOCKED PROCESSES */
        softBlockedCount--;
        /* SETTING PROCESS STATUS ON "READY" */
        insertProcQ(&readyQueue, unblockedProcess);
    }

    /* CASE = there was no process running */
    if(currentProcess == NULL)
        scheduler();
    else{
        /* LOADING OLD PROCESSOR STATE */
        LDST((state_t*) BIOSDATAPAGE);
    }
}