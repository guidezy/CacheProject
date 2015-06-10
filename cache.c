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

Cache_Error fetchDataFromFile(struct Cache* pcache, struct Cache_Block_Header* block, FILE* file, int ibfile)
{
	if(!block || !pcache || !file) return CACHE_KO;

	//Position cursor in correct position inside FILE
	if( fseek(file, DADDR(pcache, ibfile), SEEK_SET) != 0 )
		return CACHE_KO;

	//Copy BLOCKSZ bytes to block
	if( fread(block->data, pcache->recordsz, pcache->nrecords, file) != pcache->nrecords )
		return CACHE_KO;

	//Set flags
	block->flags |= VALID;
	block->flags &= ~MODIF;
	block->ibfile = ibfile;

	//Rewind file
	rewind(file);

	return CACHE_OK;
}

Cache_Error sendDataToFile(struct Cache* pcache, struct Cache_Block_Header* block, FILE* file)
{
	if(!block || !pcache || !file) return CACHE_KO;

	//Position cursor in correct position inside FILE
	if( fseek(file, DADDR(pcache, block->ibfile), SEEK_SET) != 0)
		return CACHE_KO;	

	//Copy block data to file
	if( fwrite(block->data, pcache->recordsz, pcache->nrecords, file) != pcache->nrecords)
		return CACHE_KO;

	//Set flags
	block->flags &= ~MODIF;

	//Rewind file
	rewind(file);

	return CACHE_OK;
}

void dereferenceBlocks(struct Cache* pcache)
{
	for(int i = 0; i < pcache->nblocks; i++)
		pcache->headers[i].flags &= ~R;

	pcache->instrument.n_deref++;
}

static int N_ACCESS_CACHE_SYNC = 0;
void syncTimer(struct Cache* pcache)
{
	//Timer for synchronisation
	if(N_ACCESS_CACHE_SYNC > NSYNC)
	{
		N_ACCESS_CACHE_SYNC = 0;
		Cache_Sync(pcache);
	}
	else
		N_ACCESS_CACHE_SYNC++;
}

void record2Block(struct Cache_Block_Header* block, void* record, int recordsz, int irblock)
{
	//Copy PRECORD
	unsigned long addressInBytes = recordsz * irblock;
	memcpy(&block->data[addressInBytes], record, recordsz);

	block->flags |= MODIF;
}

void block2Record(struct Cache_Block_Header* block, void* record, int recordsz, int irblock)
{
	unsigned long addressInBytes = recordsz * irblock;
	memcpy(record, &block->data[addressInBytes], recordsz);
}


typedef void (*ReflexCallback)(struct Cache *,struct Cache_Block_Header*);
typedef void (*MemTransfCallback)(struct Cache_Block_Header*,void*,int,int);
Cache_Error CacheManager(struct Cache *pcache, int irfile, const void *precord, 
	ReflexCallback reflexCallback, MemTransfCallback memTransfCallback, int* statistic)
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
		memTransfCallback(&block, precord, pcache->recordsz, irblock);
		
		//REFLEX CALL
		reflexCallback(pcache, &block);

		//Update statistics
		(*statistic)++;

		return CACHE_OK;
	}

	//NO FREE POSITION
	struct Cache_Block_Header* block = Strategy_Replace_Block(pcache);

	//Synchronize with file before substitution
	if(block->flags & MODIF)
		if( sendDataToFile(pcache, block, pcache->fp) == CACHE_KO) return CACHE_KO;

	//Fetch data from file
	if( fetchDataFromFile(pcache, block, pcache->fp, ibfile) == CACHE_KO)
		return CACHE_KO;

	//Copy block entry to buffer
	memTransfCallback(block, precord, pcache->recordsz, irblock);

	//REFLEX CALL
	reflexCallback(pcache, block);

	//Update statistics
	(*statistic)++;

	return CACHE_OK;
}


struct Cache_Block_Header* createBlocks(struct Cache* pcache)
{
	struct Cache_Block_Header* out = 
			(struct Cache_Block_Header*)malloc( sizeof(struct Cache_Block_Header) * pcache->nblocks );

	if(!out) return NULL;

	for(int i = 0; i < pcache->nblocks; i++)
	{
		out[i].ibcache = i;
		out[i].data = (char*)malloc( pcache->recordsz * pcache->nrecords );
		out[i].ibfile = -1;
	}

	return out;
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

	cache->headers = createBlocks(cache);
	if(!cache->headers) return NULL;

	cache->pfree = &cache->headers[0];

	cache->pstrategy = Strategy_Create(cache);
	printf("Cache.c is storing: %lu\n", cache->pstrategy);
	
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

	pcache = pcache->headers = NULL;

	return CACHE_OK;
}

Cache_Error Cache_Invalidate(struct Cache *pcache)
{
	if(!pcache || !pcache->headers) return CACHE_KO;

	for(int i = 0; i < pcache->nblocks; i++)
		pcache->headers[i].flags &= ~VALID;

	pcache->pfree = &pcache->headers[0];

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
	if(!pcache || !pcache->headers) return CACHE_KO;

	for(int i = 0; i < pcache->nblocks; i++)
	{
		if( sendDataToFile(pcache, &pcache->headers[i], pcache->fp) == CACHE_KO)
			return CACHE_KO;

		pcache->headers[i].flags &= ~MODIF;
	}

	N_ACCESS_CACHE_SYNC = 0;
	pcache->instrument.n_syncs++;

	return CACHE_OK;
}

Cache_Error Cache_Write(struct Cache *pcache, int irfile, const void *precord)
{	
	return CacheManager(pcache, irfile, precord, &Strategy_Write, &record2Block, &pcache->instrument.n_writes);
}

Cache_Error Cache_Read(struct Cache *pcache, int irfile, void *precord)
{
	return CacheManager(pcache, irfile, precord, &Strategy_Read, &block2Record, &pcache->instrument.n_reads);
}