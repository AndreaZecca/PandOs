#include <umps3/umps/libumps.h>
#include <umps3/umps/cp0.h>
#include "../resources/pandos_const.h"
#include "../resources/pandos_types.h"
#include "initProc.h"
#include "vmSupport.h"

//A static array of support structures to allocate
static support_t supPool[UPROCMAX];

//Master sempahore which wait for all processes to be concluded in order to terminate testing
int masterSem;

//Support structures free list
support_t* suppFree;

//Support Level devices semaphores
int devRegSem[MAX_SEMAPHORES - 1];

extern void TLB_exceptionHandler();
extern void exceptionHandler();
extern void supexHandler();
extern pcb_PTR currentProcess;
//------------------------------------------

void deallocate(support_t*  toDeallocate){
    //SAVING SUPPORT STRUCTURES LIST HEAD
    support_t *tmp;
    tmp = suppFree;
    //CASE = THERE IS NO FREE SUPPORT STRUCTURE YET
    if(tmp == NULL){
        suppFree = toDeallocate;
        suppFree->next = NULL;
    }
    else{
        //SEARCHING LAST SUPPORT STRUCTURE IN FREE LIST
        while(tmp->next != NULL)    tmp = tmp->next;
        tmp->next = toDeallocate;
        tmp = tmp->next;
        tmp->next = NULL;
    }    
}

support_t* allocate(){
    //SAVING SUPPORT STRUCTURES LIST HEAD
    support_t* tmp;
    tmp = suppFree;
    //CASE = THERE IS NO FREE SUPPORT STRUCTURE TO ALLOCATE
    if(tmp == NULL)
        return NULL;
    else{
        //REMOVING HEAD OF THE LIST
        suppFree = suppFree->next;
        tmp->next = NULL;
        return tmp;
    }    
}

void init_supLevSem(){
    for (int i = 0; i < MAX_SEMAPHORES; i += 1)
        devRegSem[i] = 1;
}

//CREATE A PROCESS USING ITS ID (PROCESS ASID)
void createProc(int id){
    //SAVING RAM-TOP ADDRESS
    memaddr ramTOP;
    RAMTOP(ramTOP);
    //FINDING THE RAM ADDRESS FOR THE ACTUAL PROCESS
    memaddr topStack = ramTOP - (2*id*PAGESIZE);

   //PROCESS STATE INITIALIZING 
    state_t newState;
    newState.entry_hi = id << ASIDSHIFT;
    newState.pc_epc = newState.reg_t9 = UPROCSTARTADDR;
    newState.reg_sp = GETSHAREFLAG;
    //ENABLING INTERRUPTS AND CREATING PROCESS IN USER-MODE
    newState.status = IMON | IEPON | TEBITON | USERPON;

    //ALLOCATING A SUPPORT STRUCT FROM FREE LIST
    support_t* supStruct = allocate();
    if(supStruct != NULL){
        //SUPPORT STRUCTURE INITAILIZING
        supStruct->sup_asid = id;
        
        //GENERAL EXCEPTION INITIALIZING
        supStruct->sup_exceptContext[GENERALEXCEPT].c_pc = (memaddr) supexHandler;
        supStruct->sup_exceptContext[GENERALEXCEPT].c_status = IMON | IEPON | TEBITON;
        supStruct->sup_exceptContext[GENERALEXCEPT].c_stackPtr = (memaddr) topStack;

        //PAGEFAULT EXCEPTION INITIALIZING
        supStruct->sup_exceptContext[PGFAULTEXCEPT].c_pc = (memaddr) TLB_exceptionHandler;
        supStruct->sup_exceptContext[PGFAULTEXCEPT].c_status = IMON | IEPON | TEBITON;
        supStruct->sup_exceptContext[PGFAULTEXCEPT].c_stackPtr = (memaddr) (topStack + PAGESIZE);

        //TLB-ENTRY INITIALIZING
        for (int i = 0; i < MAXPAGES - 1; i++)
        {
            supStruct->sup_privatePgTbl[i].pte_entryHI = 0x80000000 + (i << VPNSHIFT) + (id << ASIDSHIFT);
            supStruct->sup_privatePgTbl[i].pte_entryLO = DIRTYON;
        }
        supStruct->sup_privatePgTbl[MAXPAGES - 1].pte_entryHI = 0xBFFFF000 + (id << ASIDSHIFT);
        supStruct->sup_privatePgTbl[MAXPAGES - 1].pte_entryLO = DIRTYON;
        
        //ACTUAL PROCESS CREATION
        int status = SYSCALL(CREATEPROCESS, (memaddr) &newState, (memaddr) supStruct, 0);

        //CASE = PROCESS INCORRECTLY CREATED
        if (status != OK)
        {
            SYSCALL(TERMPROCESS, 0, 0, 0);
        }
    }
}

//TEST FUNCTION
void InstantiatorProcess(){
    //INITIALIZING SUPPORT LEVEL STRUCTURES
    init_swap();
    init_supLevSem();    
    masterSem = 0;
    suppFree = NULL;
    
    //DEALLOCATING ALL SUPPORT STRUCTURES FROM THE STATIC ARRAY
    for(int i=0; i < UPROCMAX; i++){
        deallocate(&supPool[i]);
        
    }

    //CREATING ALL THE PROCESSES
    for(int id=0; id < UPROCMAX; id++)
    {
        //User process's asid is = id+1
		createProc(id+1);
	}

    //PERFORMING A 'P' OPERATION ON MASTER SEMAPHORE FOR EACH PROCESS CREATED
    for(int i=0; i < UPROCMAX; i++)
    {
		SYSCALL(PASSEREN, (int) &masterSem, 0, 0);
	}

    //KILLING TEST PROCESS 
    SYSCALL(TERMPROCESS, 0, 0, 0);
}