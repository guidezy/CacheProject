#include "cache.h"
#include "low_cache.h"
#include "strategy.h"

#include <stdio.h>
#include <string.h>

//------------------------------
//--------- INTERNAL -----------
//------------------------------
int searchIndexInCache(int ibfile, struct Cache* pcache)
{
	for(int i = 0; i < pcache->nblocks; i++)
	{
		struct Cache_Block_Header block = pcache->headers[i];

		//Si le bloc est valide et l'indice est ce qu'on cherche,
		//retourner la position du bloc dans le cache
		if((block.flags & VALID) && block.ibfile == ibfile)
			return i;
	}

	return -1;
}

void fetchDataFromFile(struct Cache* pcache, struct Cache_Block_Header* block, FILE* file, int ibfile)
{
	//Position cursor in correct position inside FILE
	fseek(file, DADDR(pcache, ibfile), SEEK_SET);

	//Copy BLOCKSZ bytes to block
	fread(block->data, pcache->recordsz, pcache->nrecords, file);

	//Set flags
	block->flags |= VALID;
	block->flags &= ~MODIF;
	block->ibfile = ibfile;

	//Rewind file
	rewind(file);

	return;
}

void sendDataToFile(struct Cache* pcache, struct Cache_Block_Header* block, FILE* file)
{
	//Position cursor in correct position inside FILE
	fseek(file, DADDR(pcache, block->ibfile), SEEK_SET);	

	//Copy block data to file
	fwrite(block->data, pcache->recordsz, pcache->nrecords, file);

	//Rewind file
	rewind(file);
}

static int N_ACCESS_CACHE = 0;
void syncTimer(struct Cache* pcache)
{
	if(N_ACCESS_CACHE > NSYNC)
	{
		N_ACCESS_CACHE = 0;
		Cache_Sync(pcache);
	}
	else
		N_ACCESS_CACHE++;
}

void record2Block(struct Cache_Block_Header* block, void* record, int recordsz, int irblock)
{
	//Copy PRECORD
	unsigned long addressInBytes = recordsz * irblock;
	memcpy(&block->data[addressInBytes], record, recordsz);

	block->flags |= MODIF;
}

//----------------------------------
//--------- FROM CACHE.C -----------
//----------------------------------
struct Cache* Cache_Create(const char *fic, unsigned nblocks, unsigned nrecords, 
							size_t recordsz, unsigned nderef)
{
	//Ouvre le fichier FIC - et le crée si ça existe pas encore
	FILE *file;
	if( (file = fopen(fic, "r+b")) == NULL)
		file = fopen(fic, "w+b");

	//Alloue un nouveau struct Cache*
	struct Cache* cache = malloc( sizeof(struct Cache) );
	
	cache->file = fic;
	cache->fp = file;

	cache->nderef = nderef;
	cache->nblocks = nblocks;
	cache->nrecords = nrecords;
	cache->recordsz = recordsz;
	cache->blocksz = nrecords * recordsz;
	cache->instrument = (struct Cache_Instrument){0,0,0,0,0}; //Met à zero tous les champs de instrument

	cache->headers = (struct Cache_Block_Header*)malloc( sizeof(struct Cache_Block_Header) * nblocks );
	for(int i = 0; i < cache->nblocks; i++)
	{
		cache->headers[i].ibcache = i;
		cache->headers[i].data = (char*)malloc( cache->recordsz * cache->nrecords );
		cache->headers[i].ibfile = -1;
	}

	cache->pfree = &cache->headers[0];
	cache->pstrategy = Strategy_Create(cache);

	
	return cache;
}

Cache_Error Cache_Close(struct Cache *pcache)
{
	//Synchroniser les choses - faut tester si y a des erreurs retournés par Cache_Sync!
	Cache_Sync(pcache);

	//Fermer ce qu'il faut
	fclose(pcache->fp);
	Strategy_Close(pcache->pstrategy);

	//Free tous les structs
	free( pcache->headers );
	free( pcache );

	return CACHE_OK;
}

Cache_Error Cache_Invalidate(struct Cache *pcache)
{
	for(int i = 0; i < pcache->nblocks; i++)
		pcache->headers[i].flags &= ~VALID;

	Strategy_Invalidate(pcache);

	return CACHE_OK;
}

struct Cache_Instrument *Cache_Get_Instrument(struct Cache *pcache)
{
	//Copier champ INSTRUMENT de pcache
	struct Cache_Instrument* copy = (struct Cache_Instrument*)malloc( sizeof(struct Cache_Instrument) );
	memcpy( copy, &pcache->instrument, sizeof(struct Cache_Instrument) );

	//Met tous à zero
	pcache->instrument = (struct Cache_Instrument){0,0,0,0,0};

	return copy;
}

Cache_Error Cache_Sync(struct Cache *pcache)
{
	for(int i = 0; i < pcache->nblocks; i++)
	{
		sendDataToFile(pcache, &pcache->headers[i], pcache->fp);
		pcache->headers[i].flags &= ~MODIF;
	}

	pcache->instrument.n_syncs++;

	return CACHE_OK;
}

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

Cache_Error Cache_Write(struct Cache *pcache, int irfile, const void *precord)
{	
	syncTimer(pcache);

	//Calculate to which block IRFILE belongs, then verify if it's in the cache
	int ibfile = irfile / pcache->nrecords; //Number of the block which contains this entry
	int irblock = irfile % pcache->nrecords; //Index inside a block
	int blockIndex = searchIndexInCache(ibfile, pcache);

	if(blockIndex >= 0)
	{
		//Cache hit!
		pcache->instrument.n_hits++;

		//The block is inside the cache! Update value and mark it as modified
		struct Cache_Block_Header block = pcache->headers[blockIndex];
		record2Block(&block, precord, pcache->recordsz, irblock);

		//Update statistics
		pcache->instrument.n_writes++;

		return CACHE_OK;
	}

	//BLOCK IS NOT IN CACHE
	//Try to put it in some free position
	struct Cache_Block_Header* block = Get_Free_Block(pcache);

	if(block != NULL)
	{
		//We found some free space to put our data
		//Fetch data from file
		fetchDataFromFile(pcache, block, pcache->fp, ibfile);

		//Copy record to block
		record2Block(block, precord, pcache->recordsz, irblock);

		//Update statistics
		pcache->instrument.n_writes++;

		//Return
		return CACHE_OK;
	}

	//NO FREE POSITION
	block = Strategy_Replace_Block(pcache);

	//Synchronize with file before substitution
	if(block->flags & MODIF)
		sendDataToFile(pcache, block, pcache->fp);

	//Fetch data from file
	fetchDataFromFile(pcache, block, pcache->fp, ibfile);

	//Copy record to block
	record2Block(block, precord, pcache->recordsz, irblock);

	//REFLEX CALL
	Strategy_Write(pcache, block);

	//Update statistics
	pcache->instrument.n_writes++;

	//Return
	return CACHE_OK;
}

Cache_Error Cache_Read(struct Cache *pcache, int irfile, void *precord)
{
	syncTimer(pcache);

	//Calculate to which block IRFILE belongs, then verify if it's in the cache
	int ibfile = irfile / pcache->nrecords; //Number of the block which contains this entry
	int irblock = irfile % pcache->nrecords; //Index inside a block
	int blockIndex = searchIndexInCache(ibfile, pcache);

	if(blockIndex >= 0)
	{
		//Cache hit!
		pcache->instrument.n_hits++;

		//The block is inside the cache! Copy the entry to precord		
		struct Cache_Block_Header block = pcache->headers[blockIndex];
		memcpy( precord, &block.data[pcache->recordsz * irblock], pcache->recordsz );

		//Update statistics
		pcache->instrument.n_reads++;

		return CACHE_OK;
	}

	//BLOCK IS NOT IN CACHE
	//Try to fetch block in some free position
	struct Cache_Block_Header* block = Get_Free_Block(pcache);

	if(block != NULL)
	{
		//We found some free space to put our data!
		//Fetch data from file
		fetchDataFromFile(pcache, block, pcache->fp, ibfile);

		//Copy record to block
		memcpy(precord, &block->data[pcache->recordsz * irblock], pcache->recordsz);

		//Update statistics
		pcache->instrument.n_reads++;

		//Return
		return CACHE_OK;
	}

	//NO FREE POSITION
	block = Strategy_Replace_Block(pcache);

	//Synchronize with file before substitution
	if(block->flags & MODIF)
		sendDataToFile(pcache, block, pcache->fp);

	//Fetch data from file
	fetchDataFromFile(pcache, block, pcache->fp, ibfile);

	//Copy block entry to buffer
	memcpy(precord, &block->data[pcache->recordsz * irblock], pcache->recordsz);

	//REFLEX CALL
	Strategy_Read(pcache, block);

	//Update statistics
	pcache->instrument.n_reads++;

	return CACHE_OK;
}