#include <stdlib.h>
#include <stdio.h>
#include "cache_list.h"
#include "low_cache.h"


/*! Création d'une liste de blocs */
struct Cache_List *Cache_List_Create(){
	struct Cache_List * c = malloc(sizeof(struct Cache_List)); 
	c->prev = c; 
	c->next =c; 
	c->pheader=NULL; 
	return c; 
}

/*! Destruction d'une liste de blocs */
void Cache_List_Delete(struct Cache_List *list){
	struct Cache_List * iterater; 
	for(iterater=list->next; iterater!=list; iterater = iterater->next)
	{	
		iterater->prev->next = iterater ->next; 
		iterater->next->prev = iterater->prev; 
		free(iterater); 
	} 


}

/*! Insertion d'un élément à la fin */
void Cache_List_Append(struct Cache_List *list, struct Cache_Block_Header *pbh){
	struct Cache_List * new = malloc(sizeof(struct Cache_List)); 
	new->pheader=pbh; 
	struct Cache_List * debut = list->next; 
	for(; debut!=list; debut=debut->next)
	{
	}
	new->prev = debut->prev; 
	debut->prev->next = new; 
	debut->prev = new; 
	new->next = debut; 
}

/*! Insertion d'un élément au début*/
void Cache_List_Prepend(struct Cache_List *list, struct Cache_Block_Header *pbh){
	struct Cache_List * new = malloc(sizeof(struct Cache_List)); 
	new->pheader=pbh; 
	struct Cache_List * debut = list->next; 
	new->prev = debut->prev; 
	debut->prev->next = new; 
	debut->prev = new; 
	new->next = debut; 

}

/*! Retrait du premier élément */
struct Cache_Block_Header *Cache_List_Remove_First(struct Cache_List *list){
	struct Cache_List * first = list->next; 
	first->prev->next = first->next; 
	first->next->prev = first->prev; 
	return first->pheader; 
}

/*! Retrait du dernier élément */
struct Cache_Block_Header *Cache_List_Remove_Last(struct Cache_List *list){
	struct Cache_List * last = list->prev;  
	last->next->prev = last->prev; 
	last->prev->next = last->next; 
	return last->pheader; 
}

/*! Retrait d'un élément quelconque */
struct Cache_Block_Header *Cache_List_Remove(struct Cache_List *list,
                                             struct Cache_Block_Header *pbh){
	struct Cache_List * iterater; 
	for(iterater=list->next; iterater!=list && iterater->pheader!=pbh; iterater = iterater->next)
	{	

	} 
	if(iterater!=list){
		iterater->prev->next = iterater->next; 
		iterater->next->prev = iterater->prev; 
	}
	return iterater->pheader; 

}
/*! Remise en l'état de liste vide */
void Cache_List_Clear(struct Cache_List *list){
	struct Cache_List * iterater; 
	for(iterater=list->next; iterater!=list; iterater = iterater->next)
	{	
		 Cache_List_Remove(list, iterater->pheader);
	} 
}

/*! Test de liste vide */
bool Cache_List_Is_Empty(struct Cache_List *list){
	return list->prev==list->next && list->prev == list; 
}

/*! Transférer un élément à la fin */
void Cache_List_Move_To_End(struct Cache_List *list,
                            struct Cache_Block_Header *pbh)
{
	struct Cache_Block_Header *blocToDeplace = Cache_List_Remove(list,pbh);
	if(blocToDeplace!=NULL){
		Cache_List_Append(list,blocToDeplace); 
	}
}
/*! Transférer un élément  au début */
void Cache_List_Move_To_Begin(struct Cache_List *list,
                              struct Cache_Block_Header *pbh)
{
	struct Cache_Block_Header *blocToDeplace = Cache_List_Remove(list,pbh);
	if(blocToDeplace!=NULL){
		Cache_List_Prepend(list,blocToDeplace); 
	}
}

void Cache_List_Print(struct Cache_List * list){
	printf("( ");
	for(struct Cache_List * iterater=list->next; iterater!=list;iterater = iterater->next)
	 {
	 	printf("%d ",iterater->pheader->ibfile); 
	 }
	  printf(")\n"); 
}
