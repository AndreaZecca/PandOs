#include "../resources/pandos_types.h"
#include "../resources/pandos_const.h"
#include "pcb.h"
#include "asl.h"

/*Definizione di costanti*/
#define HIDDEN static
#define TRUE 1
#define FALSE 0


/*Definizione delle strutture dati utili alla gestione dei pcb*/
HIDDEN pcb_t* pcbFree_h;
HIDDEN pcb_t pcbFree_table[MAXPROC];

//1
/*
Inizializza la pcbFree in modo da contenere tutti gli elementi della pcbFree_table. Questo metodo deve essere chiamato una volta sola in fase di inizializzazione della struttura dati.
*/
void initPcbs(){ //i pcb_t nella pcbFree_table vengono inseriti nella lista dei pcb_t liberi
    pcb_PTR tmp;
    pcbFree_h = &pcbFree_table[0];
    pcbFree_h->p_prev = NULL;
    tmp = pcbFree_h;
    for(int i = 1;  i < MAXPROC; i++){
        tmp->p_next = &pcbFree_table[i];
        tmp = tmp->p_next;
        tmp->p_prev = NULL;
    }
    tmp->p_next = NULL;
}

//2
/*
Inserisce il PCB puntato da p nella lista dei PCB liberi (pcbFree_h)
*/
void freePcb(pcb_t * p){ //il pcb_t p passato come parametro viene inserito in testa alla lista dei pcb liberi
  if(p != NULL){
    p->p_next = pcbFree_h;
    p->p_prev = NULL;
    pcbFree_h = p;
  }
}


//3
/*
Restituisce NULL se la pcbFree_h è vuota. Altrimenti rimuove un elemento dalla pcbFree, inizializza tutti i campi (NULL/0) e restituisce l’elemento rimosso.
*/
pcb_t *allocPcb(){
    if(pcbFree_h == NULL) return NULL; //se pcbFree_h è vuota, restituisce NULL.
    pcb_PTR toRet = pcbFree_h;
    pcbFree_h = pcbFree_h->p_next; //l'elemento viene rimosso in testa dalla lista pcbFree_h
    return setNull(toRet); //viene ritornato con tutti i campi settati a NULL
}

/* Funzione per impostare i campi di un pcb_t a NULL/0
*/
pcb_t *setNull(pcb_PTR tmp){
    if(tmp!=NULL){
        tmp->p_next = NULL;
        tmp->p_prev = NULL;
        tmp->p_prnt = NULL;
        tmp->p_child = NULL;
        tmp->p_next_sib = NULL;
        tmp->p_prev_sib = NULL;
	      tmp->p_semAdd = NULL;
        tmp->p_time = 0;
        tmp->p_supportStruct = NULL;
        return tmp;
    }
    else{
      return NULL;
    }
}

//4
/*
Crea una lista di PCB, inizializzandola come lista vuota (i.e. restituisce NULL).
*/
pcb_t* mkEmptyProcQ(){ //crea una coda di PCB vuota
    pcb_PTR toRet = NULL;
    return toRet;
}

//5
/*
Restituisce TRUE se la lista puntata da head è vuota, FALSE altrimenti.
*/
int emptyProcQ(pcb_t *tp){
    if(tp==NULL) return TRUE; //se tp è NULL, la funzione ritorna TRUE
    else return FALSE; //altrimenti ritorna FALSE
}

//6
/*
inserisce l’elemento puntato da p nella coda dei processi tp. La doppia indirezione su tp serve per poter inserire p come ultimo elemento della coda.
*/
void insertProcQ(pcb_t **tp, pcb_t* p){
  if(tp != NULL && p != NULL){
    if(*tp == NULL){ //se la sentinella tp punta a NULL
      *tp = p; //la sentinella punta al nuovo pcb_t p
      (*tp)->p_prev = *tp; //il p_prev di p diventa p stesso (bidirezionale ciclica)
      (*tp)->p_next = *tp; //il p_next di p diventa p stesso
    }
    else{ //se la sentinella tp punta a qualcosa diverso da NULL (non vuota)
      p->p_prev = (*tp)->p_prev; //inserisco p come primo elemento
      if(p->p_prev!=NULL)
        p->p_prev->p_next = p;
      (*tp)->p_prev = p; //rendo *tp il secondo elemento
      p->p_next = *tp;
      *tp = p; //identifico p come nuovo tail pointer
    }
  }
}

//7
/*
Restituisce l’elemento in fondo alla coda dei processi tp, SENZA RIMUOVERLO. Ritorna NULL se la coda non ha elementi.
*/
pcb_t *headProcQ(pcb_t *tp){
    if(tp == NULL) return NULL; //se la coda è vuota ritorna NULL
    return tp->p_prev; //altrimenti ritorna l'elemento in fondo alla coda
}

//8
/*
Rimuove l’elemento piu’ vecchio dalla coda tp. Ritorna NULL se la coda è vuota, altrimenti ritorna il puntatore all’elemento rimosso dalla lista.
*/
pcb_t* removeProcQ(pcb_t **tp){
    if(*tp == NULL || tp == NULL){ //se la coda è vuota
      return NULL;
    }
    else{ //se la coda non è vuota
      pcb_t* tmp;
      if((*tp)->p_next == *tp){ //se è il primo elemento ad essere il piu vecchio
        tmp = *tp;
        *tp = NULL; //l'elemento viene salvato in tmp e la coda diventa vuota
      }
      else{ //se non è il primo elemento, lo rimuoviamo cambiando i puntatori p_prev e p_next dei pcb_t adiacenti
        tmp = (*tp)->p_prev;
        (*tp)->p_prev = (*tp)->p_prev->p_prev;
        (*tp)->p_prev->p_next = *tp;
        //SE NECESSARIO RITORNARE SETNULL(TMP)
      }
      return tmp;
    }
}

//9
/*
Rimuove il PCB puntato da p dalla coda dei processi puntata da tp. Se p non è presente nella coda, restituisce NULL (p può trovarsi in una posizione arbitraria della coda).
*/
pcb_t* outProcQ(pcb_t **tp, pcb_t *p){
    if(*tp == NULL || tp == NULL || p==NULL){ //se la coda è vuota oppure se il pcb_t è NULL
      return NULL; //ritorna NULL
    }
    else{
      if(*tp == p){ //se p è l'elemento in testa
        if((*tp)->p_next == p){ //se è l'unico elemento
          *tp = NULL; //la coda diventa vuota
          return p;
        }
        (*tp)->p_prev->p_next= (*tp)->p_next; //se ho più elementi
        (*tp)->p_next->p_prev= (*tp)->p_prev; //rimuovo p cambiando i puntatori p_prev e p_next dei pcb_t adiacenti
        (*tp) = (*tp)->p_next;
        return p;
      }
      else{ //se p è un elemento all'interno della coda
        pcb_t* tmp= (*tp)->p_next;
        while(tmp != p && tmp != *tp) //scorriamo tutta la coda di pcb_t *tp
          tmp= tmp->p_next;
        if(tmp == *tp) //se p non è presente
          return NULL; //ritorna NULL
        else{ //altrimenti, se p è presente
          tmp->p_prev->p_next= tmp->p_next;
          tmp->p_next->p_prev= tmp->p_prev;
          return tmp;
        }
      }
    }
}



//10
/*
Restituisce TRUE se il PCB puntato da p non ha figli, FALSE altrimenti.
*/
int emptyChild(pcb_t *p){
    if(p!= NULL && p->p_child == NULL) return TRUE; //controllo se è presente almeno un figlio
    else return FALSE;
}

//11
/*
Inserisce il PCB puntato da p come figlio del PCB puntato da prnt.
*/
void insertChild(pcb_t *prnt,pcb_t *p){
  if(prnt!=NULL && p!=NULL){
    pcb_PTR tmp = prnt->p_child; //salvo il primo figlio in tmp
    prnt->p_child = p; //inserisco p come nuovo primo figlio
    p->p_next_sib = tmp; //inserisco tmp come lista di fratelli di p
    p->p_prnt = prnt; //inserisco prnt come padre di p
    if(tmp!=NULL){ //se la lista di fratelli esiste
      tmp->p_prev_sib = p; //inserisco p come fratello precedente della lista tmp
    }
  }
}

//12
/*
Rimuove il primo figlio del PCB puntato da p. Se p non ha figli, restituisce NULL.
*/
pcb_t* removeChild(pcb_t *p){
  if(p!=NULL){
    if(p->p_child == NULL) return NULL; //se p non ha figli, return NULL
    pcb_PTR tmp = p->p_child; //salvo la lista dei figli di p in tmp
    if(tmp->p_next_sib == NULL){ //nel caso p abbia un solo figlio
      p->p_child = NULL; //rimuovo l'unico figlio
      tmp->p_prnt = NULL;
      tmp->p_prev_sib = tmp->p_next_sib = NULL;
      return tmp; //setto a null i vari campi del figlio da restituire e lo restituisco
    }
    else{ //nel caso abbia piu figli
      pcb_PTR toRet = tmp; //salvo il primo figlio in toRet
      tmp=tmp->p_next_sib;
      tmp->p_prev_sib = NULL;
      p->p_child = tmp; //il secondo figlio precedente diventa il nuovo primo figlio
      toRet->p_prnt = NULL;
      toRet->p_prev_sib = toRet->p_next_sib = NULL;
      return toRet; //restituisco il vecchio primo figlio
    }
  }
  else return NULL;
}

//13
/*
Rimuove il PCB puntato da p dalla lista dei figli del padre. Se il PCB puntato da p non ha un padre, restituisce NULL, altrimenti restituisce l’elemento rimosso (cioè p). A differenza della removeChild, p può trovarsi in una posizione arbitraria (ossia non è necessariamente il primo figlio del padre).
*/
pcb_t *outChild(pcb_t* p){
    if(p!=NULL){
      if(p->p_prnt==NULL) return NULL; //controllo se p ha un padre
      pcb_PTR list = p->p_prnt->p_child; //se p ha un padre
      if(list == p){ //se p è il primo figlio del padre
        removeChild(p->p_prnt); //uso la funzione removeChild, che rimuove il primo figlio
      }
      else{ //se p non è il primo figlio del padre
        while(list != p){ //scorro la lista di figli del padre di p
          list = list->p_next_sib;
        }
        list->p_prev_sib->p_next_sib = list->p_next_sib;
        if(list->p_next_sib != NULL)
          list->p_next_sib->p_prev_sib = list->p_prev_sib; //rimuovo p dalla lista dei figli del padre
        list->p_next_sib = list->p_prev_sib = NULL;
        list->p_prnt = NULL;
        return list; //setto a NULL i campi di p e lo restituisco
      }
    }
    else{
      return NULL;
    }
}
