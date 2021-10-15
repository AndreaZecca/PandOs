#include <umps3/umps/libumps.h>
#include <umps3/umps/cp0.h>
#include <umps3/umps/arch.h>

#include "../resources/pandos_const.h"
#include "../resources/pandos_types.h"

#include "initProc.h"
#include "sysSupport.h"
#include "vmSupport.h"

//Support Level swap pool table
HIDDEN swap_t swapTable[POOLSIZE];

//A sempahore for the swap pool table access
HIDDEN unsigned int swapSem;

extern int devRegSem[MAX_SEMAPHORES];
extern pcb_t* currentProcess;
extern int masterSem;

//OPTIMIZED TLB UPDATING 
void updateTLB(pteEntry_t *updatedEntry){
    //SEARCHING FOR THE ENTRY INTO THE TLB THANKS TO ENTRY_HI
    setENTRYHI(updatedEntry->pte_entryHI);
    TLBP();
    //CASE = THE ENTRY WAS ACTUALLY INTO THE TLB
    if((getINDEX() & PRESENTFLAG) == 0){
        //UPDATING THE ENTRY_LO REGISTER (PFN HAS BEEN UPDATED!)
        setENTRYLO(updatedEntry->pte_entryLO);
        //UPDATING THE TLB WITH THE NEW INFORMATIONS
        TLBWI();
    } 
}

//OPTIMIZED SWAP PAGE SEARCHING
int findReplace(){
    //STATIC IN ORDER TO "SAVE" THE LAST SEARCH
    static int currentReplacementIndex = 0;
    int i = 0;
    //SEARCHING FOR A FREE SWAP PAGE
    while((swapTable[(currentReplacementIndex + i) % POOLSIZE].sw_asid != NOPROC) && (i < POOLSIZE))
        ++i;
    //CASE == NO FREE SWAP PAGE    
    i = (i == POOLSIZE) ? 1 : i;
    
    return currentReplacementIndex = (currentReplacementIndex + i) % POOLSIZE;
}

//INITIALIZING SWAP POOL STRUCTURES
void init_swap(){
    for(int i= 0;i < POOLSIZE; i++){
        swapTable[i].sw_asid = NOPROC; 
    }
    swapSem = 1;
}

//PERFORMING A READ (OR WRITE) OPERATION ON A FLASH DEVICE
void writeReadFlash(unsigned int RoW, unsigned int devNumber, unsigned int occupiedPageNumber, int swap_pool_index){
    //ASSIGN FLASH DEVICE COMMAND DUE TO PAGE NUMBER AND READ OR WRITE OPERATION
    unsigned int command = (occupiedPageNumber << 8) | RoW; 
    
    //ASSIGN PHYSICAL FRAME NUMBER DUE TO SWAP POOL INDEX
    memaddr pfn = (swap_pool_index << 12) + POOLSTART;
    //PERFORMING A 'P' OPERATION ON THE ACTUAL FLASH DEVICE SEMAPHORE
    SYSCALL(PASSEREN, (memaddr) &devRegSem[DEV_INDEX(4, devNumber, FALSE)], 0, 0);
    //ASSIGNING COMMAND AND PFN TO THE ACTUAL FLASH DEVICE
    dtpreg_t* flashaddr = (dtpreg_t*) DEV_REG_ADDR(4, devNumber);
    flashaddr->data0 = pfn;
    
    //ATOMICALLY ASSIGNING COMMAND AND WAIT FOR AN INTERRUPT ON FLASH DEVICE LINE
    IEDISABLE;
    flashaddr->command = command;
    unsigned int deviceStatus = SYSCALL(IOWAIT, 4, devNumber, FALSE);
    IEENABLE;

    //PERFORMIN A 'V' OEPRATIONG ON THE ACTUAL FLASH DEVICE SEMAPHORE
    SYSCALL(VERHOGEN, (memaddr) &devRegSem[DEV_INDEX(4, devNumber, FALSE)], 0, 0);

    //CASE = OPERATION INCORRECTLY CONCLUDED
    if(deviceStatus != READY){
        killProc(&swapSem);
    }
}

//FOUND A OCCUPIED PAGE
void dirtyPage(unsigned int currASID, unsigned int occupiedPageNumber, unsigned int swap_pool_index){
    //ATOMICALLY UPDATING SWAP PAGE AND TLB
    IEDISABLE;
    swapTable[swap_pool_index].sw_pte->pte_entryLO &= ~VALIDON;
    updateTLB(swapTable[swap_pool_index].sw_pte);
    IEENABLE;
    //WRITE ON FLASH DEVICE BACKING STORE THE NEW INFORMATIONS
    writeReadFlash(FLASHWRITE, currASID-1, occupiedPageNumber, swap_pool_index); 
}

//SETTING ALL OCCUPIED PAGE OF PROCESS WITH ASID (ASID) AS UNOCCUPIED
void clearSwap(int asid){
    for(int i = 0; i < POOLSIZE; i++){
        if(swapTable[i].sw_asid == asid){
            swapTable[i].sw_asid = NOPROC;
        }
    }
}

//OPERATIONS FOR KILLING A PROCESS
void killProc(int *sem)
{
    //CLEARING ENTRY POOL
    clearSwap(currentProcess->p_supportStruct->sup_asid);

    //RELEASING MUTUAL EXCLUSION IF NEEDED
    if (sem != NULL)
    {
        SYSCALL(VERHOGEN, (int) sem, 0, 0);
    }

    //DEALLOCATING THE CURRENT PROCESS'S SUPPORT STRUCT
    deallocate(currentProcess->p_supportStruct);
    
    //PERFORMING A 'V' OPERATIONS ON THE MASTER SEMAPHORE
    SYSCALL(VERHOGEN, (int) &masterSem, 0, 0);

    //KILLING THE PROCESS
    SYSCALL(TERMPROCESS, 0, 0, 0);    
}

//PAGER
void TLB_exceptionHandler(){    
    //GETTING CURRENT PROCESS'SUPPORT STRUCT
    support_t* supp_p = (support_t*) SYSCALL(GETSUPPORTPTR, 0, 0, 0);
    //IF CAUSE == 1 KILL THE PROCESS
    if(CAUSE_GET_EXCCODE(supp_p->sup_exceptState[PGFAULTEXCEPT].cause) == 1){
        killProc(NULL);
    }
    else{
        //MUTUAL EXCLUSION ON SWAP SEMAPHORE
        SYSCALL(PASSEREN, (memaddr) &swapSem, 0, 0);
        //GETTING PAGE NUMBER
        int missingPage = GETVPN(supp_p->sup_exceptState[PGFAULTEXCEPT].entry_hi);
        //SEARCHING FOR A FREE SWAP PAGE INDEX
        int swap_pool_index = findReplace();

        //IF PAGE IS OCCUPIED
        //if(swapTable[swap_pool_index].sw_pte->pte_entryLO & VALIDON){  //PREVIOUS CHECK
        if(swapTable[swap_pool_index].sw_asid != NOPROC){
            unsigned int occupiedASID = swapTable[swap_pool_index].sw_asid;
            unsigned int occupiedPageNumber = swapTable[swap_pool_index].sw_pageNo;            
            dirtyPage(occupiedASID, occupiedPageNumber, swap_pool_index);
        }

        //READING THE CONTENTS OF THE CURRENT PROCESS'S BACKING STORE/FLASH DEVICE LOGICAL PAGE X INTO THE FRAME I
        writeReadFlash(FLASHREAD, supp_p->sup_asid-1, missingPage, swap_pool_index);

        //ATOMIC OPERATIONS
        IEDISABLE;
        //UPDATING SWAP POOL TABLE ENTRY TO REFLECT THE NEW CONTENTS
        swapTable[swap_pool_index].sw_asid = supp_p->sup_asid;
        swapTable[swap_pool_index].sw_pageNo = missingPage;
        swapTable[swap_pool_index].sw_pte = &(supp_p->sup_privatePgTbl[missingPage]);

        //UPDATING THE CURRENT PROCESS'S PAGE TABLE TO INDICATE IT'S NOW VALID 
        supp_p->sup_privatePgTbl[missingPage].pte_entryLO |= VALIDON;
        supp_p->sup_privatePgTbl[missingPage].pte_entryLO = (supp_p->sup_privatePgTbl[missingPage].pte_entryLO & ~ENTRYLO_PFN_MASK) | ((swap_pool_index << 12) + POOLSTART);
        
        updateTLB(&(supp_p->sup_privatePgTbl[missingPage]));
        //END OF ATOMIC OPERATIONS
        IEENABLE;
        //RELEASING MUTUAL EXCLUSION
        SYSCALL(VERHOGEN, (memaddr) &swapSem, 0, 0);
        //LOADING PREVIOUS STATE
        LDST(&(supp_p->sup_exceptState[PGFAULTEXCEPT]));
    }
}

//GETTING ENTRY ASSOCIATED TO A PAGE NUMBER
pteEntry_t *findEntry(unsigned int pageNumber){
    //WE CAN FIND THE ENTRY IN THE SUP_PRIVATE PAGE TABLE, IN THE CURRENT PROCESS'S SUPPORT STRUCT
    return &(currentProcess->p_supportStruct->sup_privatePgTbl[pageNumber]);
}

//TLB REFILL FUNCTION
void uTLB_refillHandler(){
    //SAVING EXCEPTION STATE FROM BIOSDATAPAGE
    state_t* ex_state = (state_t*)BIOSDATAPAGE;
    //GETTING PAGE NUMBER
    int pg = GETVPN(ex_state->entry_hi);
    //SETTING ENTRY_HY AND ENTRY_LO REGISTERS
    setENTRYHI(currentProcess->p_supportStruct->sup_privatePgTbl[pg].pte_entryHI);
    setENTRYLO(currentProcess->p_supportStruct->sup_privatePgTbl[pg].pte_entryLO);
    //WRITING ON TLB
    TLBWR();        
    //LOADING PREVIOUS STATE                                                                                                                                                                                                                                                                                                                                                                                                      
    LDST(ex_state);
}       
