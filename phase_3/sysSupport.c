#include <umps3/umps/arch.h>
#include <stdbool.h>
#include <umps3/umps/libumps.h>
#include <umps3/umps/cp0.h>
#include "../resources/pandos_const.h"
#include "../resources/pandos_types.h"
#include "vmSupport.h"
#include "sysSupport.h"

extern int devRegSem[MAX_SEMAPHORES];
extern pcb_PTR currentProcess;

//SUPPORT LEVEL SYSCALL HANDLER
void supexHandler(){
    //GETTING SUPPORT STRUCT OF CURRENT PROCESS 
    support_t* except_supp = (support_t*) SYSCALL(GETSUPPORTPTR, 0, 0, 0);
    //GETTING STATE REGISTER
    state_t* exc_state = (state_t*) &(except_supp->sup_exceptState[GENERALEXCEPT]);
    //CHECKING THE CAUSE CODE (IF 8 -> SYSCALL)
    if(CAUSE_GET_EXCCODE(exc_state->cause) == 8){
        //CALLING SYSHANDLER OF SUPPORT LEVEL
        sysHandler(except_supp,  exc_state, exc_state->reg_a0);
    }
    else{
        //TERMINATING CURRENT PROCESS
        killProc(NULL);
    }
}

void sysHandler(support_t* except_supp, state_t* exc_state, unsigned int sysNum){
    char* toRead;
    int index;
    int response;
    char recvChar;
    switch(sysNum){
        case TERMINATE:
            terminate();
            break;
        case GET_TOD:
            get_tod(exc_state);
            break;
        case WRITEPRINTER:
            write_printer(except_supp, exc_state);
            break;
        case WRITETERMINAL:
            write_terminal(except_supp, exc_state);
            break;
        case READTERMINAL:
            read_terminal(except_supp, exc_state);
            break;
        default:
            killProc(NULL);
            break;
    }
    LDST(exc_state);
}

//SYSCALL 9: TERMINATE PROCESS
void terminate(){
    killProc(NULL);
}

//SYSCALL 10: GETTING CURRENT TOD
void get_tod(state_t* exc_state){
    cpu_t tod;
    STCK(tod);
    //RETURNING CURRENT TOD IN V0 REGISTER
    exc_state->reg_v0 = tod;
}

//SYSCALL 11: WRITE ON PRINTER DEVICE
void write_printer(support_t* except_supp, state_t* exc_state){
    //ASID GOES FROM 1 TO 8, WHILE DEVICES GOES FROM 0 TO 7
    int asid = except_supp->sup_asid - 1;
    //ADDRESS OF THE STRING
    char* toPrint = (char*) exc_state->reg_a1;
    //LENGTH OF THE STRING
    int len = exc_state->reg_a2;
    //CHECKING IF LENGTH IS<0 OR >128 OR IF ADDRESS OF THE STRING IS OUTSIDE OF USER PROCESS MEMORY SECTION
    if((int)toPrint < KUSEG || len < 0 || len > MAXSTRLEN)
        killProc(NULL);
    else{
        //SAVING DEVICE REGISTER
        dtpreg_t* currDev = (dtpreg_t*) DEV_REG_ADDR(PRNTINT, asid);
        //MUTUAL EXCLUSION
        SYSCALL(PASSEREN, (memaddr) &devRegSem[DEV_INDEX(PRNTINT, asid, FALSE)], 0, 0);
        int index = 0;
        int response = 1;
        while(index < len){
            //DISABLING INTERRUPT FOR ATOMIC OPERATIONS
            IEDISABLE;
            //GIVING INTERRUPT DATA
            currDev->data0 = *toPrint;
            //GIVING INTERRUPT COMMAND
            currDev->command = TRANSMITCHAR;
            //WAITING FOR IO INTERRUPT
            response = SYSCALL(IOWAIT, PRNTINT, asid, FALSE);
            //ENABLING INTERRUPT AFTER ATOMIC OPERATIONS
            IEENABLE;
            //CHECKING RESPONSE, IF READY, WE CAN PROCEDE
            if((response & 0x000000FF) == READY){
                index++;
                toPrint++;  
            }
            else{
                //ELSE, RETURNING THE NEGATIVE OF THE RESPONSE STATUS
                index = -(response & 0x000000FF);
                break;
            }
        }
        //RELEASING MUTUAL EXCLUSION
        SYSCALL(VERHOGEN, (memaddr) &devRegSem[DEV_INDEX(PRNTINT, asid, FALSE)], 0, 0);
        //RETURNING THE INDEX ON THE V0 REGISTER
        exc_state->reg_v0 = index;
    }
} 

void write_terminal(support_t* except_supp, state_t* exc_state){
    //ASID GOES FROM 1 TO 8, WHILE DEVICES GOES FROM 0 TO 7
    int asid = except_supp->sup_asid - 1;
    //ADDRESS OF THE STRING
    char* toWrite = (char*) exc_state->reg_a1;
    //LENGTH OF THE STRING
    int len = exc_state->reg_a2;
    //CHECKING IF LENGTH IS<0 OR >128 OR IF ADDRESS OF THE STRING IS OUTSIDE OF USER PROCESS MEMORY SECTION
    if(len <= 0 || (int)toWrite < KUSEG || len >= MAXSTRLEN){
        killProc(NULL);
    }
    else{
        //SAVING DEVICE REGISTER
        devreg_t* currDev = (devreg_t*) DEV_REG_ADDR(TERMINT, asid);
        //MUTUAL EXCLUSION
        SYSCALL(PASSEREN, (memaddr) &devRegSem[DEV_INDEX(TERMINT, asid, FALSE)], 0, 0);
        int index = 0;
        int response = 1;
        while(index < len){
            //GIVING INTERRUPT COMMAND   
            currDev->term.transm_command =  (*toWrite<<8) | 2;
            //WAITING FOR IO INTERRUPT
            response = SYSCALL(IOWAIT, 7, asid, FALSE);
            //CHECKING RESPONSE, IF READY, WE CAN PROCEDE
            if((response & TRANSM_MASK) == OKCHARTRANS){
                index++;
                toWrite++;
            }
            else{
                //ELSE, RETURNING THE NEGATIVE OF THE RESPONSE STATUS
                index = -(response & TRANSM_MASK);
                break;
            }
        }
        //RELEASING MUTUAL EXCLUSION
        SYSCALL(VERHOGEN, (memaddr) &devRegSem[DEV_INDEX(TERMINT, asid, FALSE)], 0, 0);
        //RETURNING THE INDEX ON THE V0 REGISTER
        exc_state->reg_v0 = index;
    }
}

void read_terminal(support_t* except_supp, state_t* exc_state){
    //ASID GOES FROM 1 TO 8, WHILE DEVICES GOES FROM 0 TO 7
    int asid = except_supp->sup_asid - 1;
    //ADDRESS OF THE STRING
    char* toRead = (char*) exc_state->reg_a1;
    int index = 0, response;
    //CHECKING IF THE ADDRESS OF THE STRING IS OUTSIDE OF USER PROCESS MEMORY SECTION
    if((int) toRead < KUSEG)
        killProc(NULL);
    else{
        //SAVING DEVICE REGISTER
        devreg_t* currDev = (devreg_t*) DEV_REG_ADDR(TERMINT, asid);
        //MUTUAL EXCLUSION
        SYSCALL(PASSEREN, (memaddr) &devRegSem[DEV_INDEX(TERMINT, asid, TRUE)], 0, 0);
        while(TRUE){
            //GIVING INTERRUPT COMMAND   
            currDev->term.recv_command = TRANSMITCHAR;
            //WAITING FOR IO INTERRUPT
            response = SYSCALL(IOWAIT, TERMINT, asid, TRUE);
            //CHECKING RESPONSE, IF OK, WE CAN PROCEDE
            if((response & RECV_MASK) == OKCHARTRANS){
                //ADDING THE UPCOMING CHARACTER IN THE STRING
                *toRead = (response & 0x0000FF00) >> BYTELENGTH;
                toRead++;
                index++;
                //WE CAN TERMINATE THE READING
                if(((response & 0x0000FF00) >> BYTELENGTH) == '\n'){
                    break;
                }
            } else {
                //ELSE, RETURNING THE NEGATIVE OF THE RESPONSE STATUS
                index = -(response & RECV_MASK);
                break;
            }
        }
        //RELEASING MUTUAL EXCLUSION
        SYSCALL(VERHOGEN, (memaddr) &devRegSem[DEV_INDEX(TERMINT, asid, TRUE)], 0, 0);
        //RETURNING THE INDEX ON THE V0 REGISTER
        exc_state->reg_v0 = index;
    }
}