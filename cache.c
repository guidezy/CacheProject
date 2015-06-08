#include "cache.h"
#include "low_cache.h"
#include "strategy.h"

#include <stdio.h>

//------------------------------
//--------- INTERNAL -----------
//------------------------------


//----------------------------------
//--------- FROM CACHE.C -----------
//----------------------------------
struct Cache* Cache_Create(const char *fic, unsigned nblocks, unsigned nrecords, 
							size_t recordsz, unsigned nderef)
{
	//Open file FIC if it does not already exist
	FILE *file;
	if( (file = fopen(fic, "r+")) == NULL)
		file = fopen(fic, "w+");

	//Allocate a new struct Cache*
	struct Cache* cache = malloc( sizeof(struct Cache) );
	
	cache->file = fic;
	cache->fp = file;

	cache->nderef = nderef;
	cache->nblocks = nblocks;
	cache->nrecords = nrecords;
	cache->recordsz = recordsz;
	cache->blocksz = nrecords * recordsz;
	cache->instrument = (Cache_Instrument*)0; //Met à zero tous les champs de instrument

	cache->headers = (struct Cache_Block_Header*)malloc( cache->blocksz * nblocks );
	cache->pfree = cache->headers[0];
	cache->pstrategy = Strategy_Create(cache);

	return cache;
}