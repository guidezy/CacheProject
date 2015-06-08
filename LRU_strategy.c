/*!
 * \file LRU.c
 * Implementation de la strategie LRU 

 */

#include <assert.h>

#include "strategy.h"
#include "low_cache.h"
#include "random.h"
#include "time.h"
#include "cache_list.h"

void *Strategy_Create(struct Cache *pcache) 
{
    struct Cache_List *liste = Cache_List_Create();
    pcache->pstrategy = liste; 
    return liste; 
}


void Strategy_Close(struct Cache *pcache)
{
	  Cache_List_Delete(pcache->pstrategy); 
	  
}


void Strategy_Invalidate(struct Cache *pcache)
{
    Cache_List_Clear(pcache->pstrategy); 

}


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


void Strategy_Read(struct Cache *pcache, struct Cache_Block_Header *pbh) 
{
	Cache_List_Move_To_End(pcache->pstrategy, pbh); 
}  

 
void Strategy_Write(struct Cache *pcache, struct Cache_Block_Header *pbh)
{
	Cache_List_Move_To_End(pcache->pstrategy, pbh); 
} 

char *Strategy_Name()
{
    return "LRU";
}
