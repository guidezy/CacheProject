#include "low_cache.h"

struct Cache_Block_Header *Get_Free_Block(struct Cache *pcache)
{
	struct Cache_Block_Header* free = pcache->pfree;

	//If FREE = null, all blocks are being used. Return NULL
	if(!free) return NULL;

	//Mark this block as being used
	free->flags |= VALID;

	//Search next free block
	int i;
	for(i = 0; i < pcache->nblocks; i++)
		if( !(pcache->headers[i].flags & VALID) )
		{
			pcache->pfree = &pcache->headers[i];
			break;
		}

	if(i == pcache->nblocks)
		pcache->pfree = NULL; //All blocks are busy. The next free is NULL, then

	return free;
}