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
    return liste; 
}


void Strategy_Close(struct Cache *pcache)
{
	 Cache_List_Delete(pcache->pstrategy); 
}


void Strategy_Invalidate(struct Cache *pcache)
{
    struct Cache_Block_Header *pbh;
    pbh = Get_Free_Block(pcache);
    Cache_List_Append(pcache->pstrategy, pbh); 

}


struct Cache_Block_Header *Strategy_Replace_Block(struct Cache *pcache) 
{

    return Cache_List_Remove_First(pcache->pstrategy); 
}


void Strategy_Read(struct Cache *pcache, struct Cache_Block_Header *pbh) 
{
	Cache_List_Append(pcache->pstrategy, pbh); 
}  

 
void Strategy_Write(struct Cache *pcache, struct Cache_Block_Header *pbh)
{
	Cache_List_Append(pcache->pstrategy, pbh); 
} 

char *Strategy_Name()
{
    return "LRU";
}
