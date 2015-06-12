/*!
 * \file FIFO.c
 *
 * \brief  Stratégie de remplacement fifo..
 * 
 */

#include <assert.h>

#include "strategy.h"
#include "low_cache.h"
#include "random.h"
#include "time.h"
#include "cache_list.h"

/*
* Initialisation de la liste qui contiendra les blocs
*
*/
void *Strategy_Create(struct Cache *pcache) 
{
    struct Cache_List *liste = Cache_List_Create();
    return (struct Cache_List*)liste;
}

/*
* On vide completement la liste de bloc et on la detruit
*
*/
void Strategy_Close(struct Cache *pcache)
{
	Cache_List_Delete(pcache->pstrategy);
}

/*
* On nettoi la liste de bloc, elle devient vide
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
    struct Cache_Block_Header *pbh;

    /* On cherche d'abord un bloc invalide */
    if ((pbh = Get_Free_Block(pcache)) != NULL) {
    	Cache_List_Append(pcache->pstrategy,pbh);
    	return pbh;
    }
    pbh=Cache_List_Remove_First(pcache->pstrategy);
	Cache_List_Append(pcache->pstrategy, pbh);
    
    return pbh;
}

/*
 * RAND : Rien à faire ici.
 */
void Strategy_Read(struct Cache *pcache, struct Cache_Block_Header *pbh) 
{
}  

/*
 * RAND : Rien à faire ici.
 */ 
void Strategy_Write(struct Cache *pcache, struct Cache_Block_Header *pbh)
{
} 

/*
* Retourne le nom de la stragie utilisé
*
*/
char *Strategy_Name()
{
    return "FIFO";
}
