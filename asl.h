#ifndef ASL_H_INCLUDED
#define ASL_H_INCLUDED

/* FUNZIONI GESTIONE DELLE ASL */

/* 14 */ int insertBlocked(int *semAdd,pcb_t *p);

/* 15 */ pcb_t* removeBlocked(int *semAdd);

/* 16 */ pcb_t* outBlocked(pcb_t *p);

/* 17 */ pcb_t* headBlocked(int *semAdd);

/* 18 */ void initASL();

//FUNZIONI AUSILIARIE
/*Funzione per l'inserimento di un elemento in una lista monodirezionale ordinata*/
semd_PTR recursiveSortedInsert(semd_PTR head, semd_PTR toAdd);

/*Funzione per la rimozione di un elemento da una lista monodirezionale*/
semd_PTR removeRecursive(semd_PTR head, int* addRem);

#endif
