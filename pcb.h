#ifndef PCB_H_INCLUDED
#define PCB_H_INCLUDED
/* FUNZIONI ALLOCAZIONE DEI PCB */

/* 1 */ void initPcbs();

/* 2 */ void freePcb(pcb_t * p);

/* 3 */ pcb_t* allocPcb();

/* 4 */ pcb_t* mkEmptyProcQ();

/* 5 */ int emptyProcQ(pcb_t *tp);

/* 6 */ void insertProcQ(pcb_t **tp, pcb_t* p);

/* 7 */ pcb_t* headProcQ(pcb_t *tp);

/* 8 */ pcb_t* removeProcQ(pcb_t **tp);

/* 9 */ pcb_t* outProcQ(pcb_t **tp, pcb_t *p);

/* FUNZIONA LISTA DEI PCB */

/* 10 */ int emptyChild(pcb_t *p);

/* 11 */ void insertChild(pcb_t *prnt,pcb_t *p);

/* 12 */ pcb_t* removeChild(pcb_t *p);

/* 13 */ pcb_t *outChild(pcb_t* p);


//FUNZIONI AUSILIARIE
/*Funzione che setta a NULL tutti i puntatori relativi al pcb_PTR passato come parametro*/
pcb_t *setNull(pcb_PTR tmp);
#endif
