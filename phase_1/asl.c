#include "../resources/pandos_types.h"
#include "../resources/pandos_const.h"
#include "pcb.h"
#include "asl.h"

/*Definizione di costanti*/
#define HIDDEN static
#define TRUE 1
#define FALSE 0
#define MAXINT 0xFFFFFFFF

/*Definizione delle strutture dati utili alla gestione dei semafori*/
static semd_t semd_table[MAXPROC + 2];
static semd_PTR semdFree_h;
static semd_PTR semd_h;



//14
/*
Viene inserito il PCB puntato da p nella coda dei processi bloccati associata al SEMD con chiave semAdd. Se il semaforo corrispondente non è presente nella ASL, alloca un nuovo SEMD dalla lista di quelli liberi (semdFree) e lo inserisce nella ASL, settando I campi in maniera opportuna (i.e. key e s_procQ). Se non è possibile allocare un nuovo SEMD perché la lista di quelli liberi è vuota, restituisce TRUE. In tutti gli altri casi, restituisce FALSE.
*/
int insertBlocked(int *semAdd, pcb_t *p){
  semd_PTR head = semd_h;
  while(head != NULL && head->s_semAdd != semAdd){
    head= head->s_next;      //Scorro la lista degli ASL per cercare il semaforo in cui inserire il processo p
  }
  if(head != NULL){  //Semaforo trovato
    p->p_semAdd = semAdd;
    insertProcQ(&(head->s_procQ), p);  //Sfrutto la insertProcQ definita per le operazioni sulle code di pcb
    return FALSE;
  }
  else{  //Semaforo non trovato
    if(semdFree_h == NULL){  //Non ho semafori nella lista libera, non posso allocarlo
      return TRUE;
    }
    else{  //Alloco un nuovo semaforo inizializzando col semAdd passato come parametro
      semd_PTR toAlloc= semdFree_h;
      toAlloc->s_semAdd = semAdd;
      p->p_semAdd = semAdd;
      toAlloc->s_procQ = NULL;
      insertProcQ(&(toAlloc->s_procQ), p);
      semdFree_h = semdFree_h->s_next;
      semd_h= recursiveSortedInsert(semd_h, toAlloc);   //Inserisco il semaforo allocato nella lista degli ASL, tenendola ordinata
      return FALSE;
    }
  }
}


/*Funzione per l'inserimento di un semaforo all'interno di una lista
La funzione conserva l'ordine della lista*/
semd_PTR recursiveSortedInsert(semd_PTR head, semd_PTR toAdd){
  if(head==NULL){
    toAdd->s_next = NULL;
    return toAdd;
  }
  else{
    if(toAdd->s_semAdd < head->s_semAdd){
      toAdd->s_next = head;
      return toAdd;
    }
    else{
      head->s_next = recursiveSortedInsert(head->s_next, toAdd);
      return head;
    }
  }
}

//15
/*
Ritorna il primo PCB dalla coda dei processi bloccati (s_procq) associata al SEMD della ASL con chiave semAdd. Se tale descrittore non esiste nella ASL, restituisce NULL. Altrimenti, restituisce l’elemento rimosso. Se la coda dei processi bloccati per il semaforo diventa vuota, rimuove il descrittore corrispondente dalla ASL e lo inserisce nella coda dei descrittori liberi (semdFree_h).
*/
pcb_t* removeBlocked(int *semAdd){
  semd_PTR tmp = semd_h;
  while(tmp != NULL){  //Scorro la lista dei semafori attivi ASL
    if(tmp->s_semAdd == semAdd){ //Trovo il semaforo corrispondete a semAdd
      pcb_PTR toRet = removeProcQ(&(tmp->s_procQ));  //rimuovo l'elemento dalla s_procQ del semaforo
      if(emptyProcQ(tmp->s_procQ)){  //se s_procQ del semaforo è vuota allora lo restituisco alla lista libera semdFree_h
        semd_h= removeRecursive(semd_h, tmp->s_semAdd);
        semdFree_h = recursiveSortedInsert(semdFree_h, tmp); //mantengo semdFree_h ordinata
      }
      if(toRet != NULL) //se viene rimosso un elemento, se ne fa il return
        toRet->p_semAdd = NULL;
      return toRet;
    }
    tmp = tmp->s_next;
  }
  return NULL; //se il descrittore relativo non esiste, viene restituito NULL
}

/*Funzione ricorsiva per la rimozione di un elemento da una lista monodirezionale*/
semd_PTR removeRecursive(semd_PTR head, int* addRem){
  if(head == NULL) return NULL;
  if(head->s_semAdd == addRem) return head->s_next;
  else{
    head->s_next = removeRecursive(head->s_next, addRem);
    return head;
  }
}

//16
/*
Rimuove il PCB puntato da p dalla coda del semaforo su cui è bloccato (indicato da p->p_semAdd). Se il PCB non compare in tale coda, allora restituisce NULL (condizione di errore). Altrimenti, restituisce p.
*/
pcb_t* outBlocked(pcb_t *p){
  semd_PTR tmpS = semd_h;
  while(tmpS!=NULL && tmpS->s_semAdd != p->p_semAdd){
    tmpS= tmpS->s_next;   //Scorro la ASL fino a cercare il semaforo in cui si trova bloccato il processo p
  }
  if(tmpS != NULL){  //Se trovo il semaforo
    pcb_PTR toRet = outProcQ(&(tmpS->s_procQ), p);  //rimuovo il processo p dalla s_procQ del semaforo
    if(toRet!=NULL){
      if(emptyProcQ(tmpS->s_procQ)){   //se s_procQ del semaforo è vuota allora lo restituisco alla lista libera semdFree_h
        semd_h = removeRecursive(semd_h, tmpS->s_semAdd);
        semdFree_h = recursiveSortedInsert(semdFree_h, tmpS);
      }
    }
    return toRet;
  }
  else return NULL; //Se non trovo il semaforo
}

//17
/*
Restituisce (senza rimuovere) il puntatore al PCB che si trova in testa alla coda dei processi associata al SEMD con chiave semAdd. Ritorna NULL se il SEMD non compare nella ASL oppure se compare ma la sua coda dei processi è vuota.
*/
pcb_t* headBlocked(int *semAdd){
  semd_PTR tmpS = semd_h;
  while (tmpS != NULL && tmpS->s_semAdd != semAdd){ //Scorro la lista dei semafori ASL
    tmpS= tmpS->s_next;
  }
  if(tmpS != NULL) {  //Trovo il semaforo corrispondete a semAdd
    if(emptyProcQ(tmpS->s_procQ)) return NULL;  //Se s_procQ vuota restituisco NULL
    return tmpS->s_procQ->p_prev; //altrimenti restituisco il processo in testa nella s_procQ (tail pointer)
  }
  else return NULL;
}

//18
/*
Inizializza la lista dei semdFree in modo da contenere tutti gli elementi della semdTable. Questo metodo viene invocato una volta sola durante l’inizializzazione della struttura dati.
*/
void initASL(){  //Inizializzo le liste semd_h e semdFree_h
  semd_h = &(semd_table[0]);
  semd_h->s_semAdd = (unsigned int *)(0x00000000);  //valore 0 per il primo dummy node
  semd_h->s_procQ = NULL;

  semd_h->s_next = &(semd_table[MAXPROC+1]);
  semd_h->s_next->s_procQ = NULL;
  semd_h->s_next->s_next = NULL;
  semd_h->s_next->s_semAdd = (unsigned int *)MAXINT; //valore MAXINT per l'ultimo dummy node

  semd_PTR tmp;
  semdFree_h = &semd_table[1];
  tmp = semdFree_h;
  for(int i=2; i<MAXPROC+1; i++){
    tmp->s_next = &(semd_table[i]);
    tmp->s_procQ = NULL;
    tmp = tmp->s_next;
  }
  tmp->s_procQ = NULL;
  tmp->s_next = NULL;
}
