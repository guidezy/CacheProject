/*!
 * \file LRU.c
 * Implementation de la strategie LRU 
 *
 */

#include <assert.h>

#include "strategy.h"
#include "low_cache.h"
#include "random.h"
#include "time.h"
#include "cache_list.h"

/*
*
* Initialisation de la liste
*
*/
void *Strategy_Create(struct Cache *pcache) 
{
    struct Cache_List *liste = Cache_List_Create(); 
    return liste; 
}


/**
* On detruit completement la liste
*
*/
void Strategy_Close(struct Cache *pcache)
{
    Cache_List_Delete(pcache->pstrategy); 
	  
}

/*
* On nettoi la liste, elle devient vide
*
*/
void Strategy_Invalidate(struct Cache *pcache)
{
    Cache_List_Clear(pcache->pstrategy); 

}

/*
* Si il y a un block libre, on l'ajout au fond de la liste et on le renvoi. 
* Sinon, on ajout le première bloc de la liste au fond de la liste et on le renvoi : c'est celui qui a été utilisé le moins recemment.
*
*/
struct Cache_Block_Header *Strategy_Replace_Block(struct Cache *pcache) 
{
    struct Cache_Block_Header * pbh; 
    if ((pbh = Get_Free_Block(pcache)) != NULL){
	Cache_List_Append(pcache->pstrategy, pbh); 
	return pbh; 
    }
    pbh= Cache_List_Remove_First(pcache->pstrategy);
    Cache_List_Append(pcache->pstrategy, pbh); 
    return pbh; 
}

/*
* Dans la stratégie LRU au moment de la lecture on deplace le bloc lu a la fin de la liste.
*
*/
void Strategy_Read(struct Cache *pcache, struct Cache_Block_Header *pbh) 
{
     Cache_List_Move_To_End(pcache->pstrategy, pbh); 
}  

/*
* Dans la stratégie LRU au moment de l'écriture on deplace le bloc lu a la fin de la liste.
*
*/
void Strategy_Write(struct Cache *pcache, struct Cache_Block_Header *pbh)
{
     Cache_List_Move_To_End(pcache->pstrategy, pbh); 
} 

/*
* Retourne le nom de la stragie utilisé
*
*/
char *Strategy_Name()
{
    return "LRU";
}
