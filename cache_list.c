#include <stdlib.h>
#include <stdio.h>
#include "cache_list.h"


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
	struct Cache_List * iterater; 
	for(iterater=list->next; iterater!=list; iterater = iterater->next)
	{	
	} 
	struct Cache_List * new = malloc(sizeof(struct Cache_List)); 
	new->pheader = pbh;
	new->next = iterater; 
	new->prev = iterater->prev; 
	
	iterater->prev->next = new;
	iterater->prev = new;
}

/*! Insertion d'un élément au début*/
void Cache_List_Prepend(struct Cache_List *list, struct Cache_Block_Header *pbh){
	struct Cache_List * new = malloc(sizeof(struct Cache_List)); 
	new->pheader=pbh; 
	new->next = list; 
	new->prev = list->prev; 
	
	list->prev->next = new;
	list->prev = new;

}

/*! Retrait du premier élément */
struct Cache_Block_Header *Cache_List_Remove_First(struct Cache_List *list){
	struct Cache_List * first = list; 
	first->prev->next = first->next; 
	first->next->prev = first->prev; 
	return first->pheader; 
}

/*! Retrait du dernier élément */
struct Cache_Block_Header *Cache_List_Remove_Last(struct Cache_List *list){
	struct Cache_List * iterater; 
	for(iterater=list->next; iterater!=list; iterater = iterater->next)
	{	
	} 
	iterater->prev->next = iterater->next; 
	iterater->next->prev = iterater->prev; 
	return iterater->pheader; 
}

/*! Retrait d'un élément quelconque */
struct Cache_Block_Header *Cache_List_Remove(struct Cache_List *list,
                                             struct Cache_Block_Header *pbh){
	struct Cache_List * iterater; 
	for(iterater=list->next; iterater!=list && iterater->pheader!=pbh; iterater = iterater->next)
	{	

	} 
	iterater->prev->next = iterater->next; 
	iterater->next->prev = iterater->prev; 
	return iterater->pheader; 

}
/*! Remise en l'état de liste vide */
void Cache_List_Clear(struct Cache_List *list){
	while(list->pheader != NULL) Cache_List_Remove_Last(list);
}

/*! Test de liste vide */
bool Cache_List_Is_Empty(struct Cache_List *list){
	if(list->pheader == NULL) return true;

	return false;
}

/*! Transférer un élément à la fin */
void Cache_List_Move_To_End(struct Cache_List *list,
                            struct Cache_Block_Header *pbh)
{
	if(list->prev->pheader == pbh) return;
	struct Cache_List iterater;
	for(iterater = list->next; iterater != list;iterater = iterater->next){
		if(pbh == iterater->pheader){
			Cache_List_Remove(list,pbh);
		}
	}
	pbh->prev = iterater;
	pbh->next = list;
	list->prev = pbh;
	iterater->next = pbh;
}
/*! Transférer un élément  au début */
void Cache_List_Move_To_Begin(struct Cache_List *list,
                              struct Cache_Block_Header *pbh)
{
	if(list->pheader == pbh) return;
	struct Cache_List iterater;
	for(iterater = list->next; iterater != list;iterater = iterater->next){
		if(pbh == iterater->pheader){
			Cache_List_Remove(list,pbh);
		}
	}
	pbh->prev = iterater;
	pbh->next = list;
	list->prev = pbh;
	iterater->next = pbh;
}

/*
void Cache_List_Print(struct Cache_List * list){
	printf("( ");
	for(struct Cache_List * iterater=list->next; iterater!=list;iterater = iterater->next)
	 {
	 	printf("%d ",iterater->pheader->ibfile); 
	 }
	  printf(")\n"); 

}*/
