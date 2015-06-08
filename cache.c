#include "cache.h"
#include "low_cache.h"
#include "strategy.h"

#include <stdio.h>
#include <stdlib.h>

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
	fread(block->data, pcache->recordsz, 1, file);

	return;
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

	cache->headers = (struct Cache_Block_Header*)malloc( cache->blocksz * nblocks );
	cache->pfree = &cache->headers[0];
	cache->pstrategy = Strategy_Create(cache);

	
	return cache;
}

Cache_Error Cache_Close(struct Cache *pcache)
{
	//Synchroniser les choses - faut tester si y a des erreurs retournés par Cache_Sync!
	Cache_Sync(pcache);

	//Fermer les fichiers
	fclose(pcache->fp);

	//Free tous les structs
	free( pcache->headers );
	free( pcache );

	return CACHE_OK;
}

Cache_Error Cache_Invalidate(struct Cache *pcache)
{
	for(int i = 0; i < pcache->nblocks; i++)
		pcache->headers[i].flags = pcache->headers[i].flags & !VALID;
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

Cache_Error Cache_Write(struct Cache *pcache, int irfile, const void *precord)
{	
	//Calculate to which block IRFILE belongs, then verify if it's in the cache
	int ibfile = irfile / pcache->nrecords; //Number of the block which contains this entry
	int irblock = irfile % pcache->nrecords; //Index inside a block
	int blockIndex = searchIndexInCache(ibfile, pcache);

	if(blockIndex >= 0)
	{
		//The block is inside the cache! Update value and mark it as modified
		struct Cache_Block_Header block = pcache->headers[blockIndex];

		memcpy( block.data[irblock], precord, pcache->recordsz );
		block.flags = block.flags | MODIF;

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
		memcpy(block->data[irblock], precord, pcache->recordsz);

		//Mark it as modified
		block->flags = block->flags | MODIF;

		//Return
		return CACHE_OK;
	}

	/*
	— Si le cache ne contient pas l’enregistrement demandé, on cherche un bloc libre (i.e.,
	invalide V = 0) dans le cache et y copie tout le bloc du fichier contenant l’enregistrement
	d’index (irfile) ; on peut alors effectuer le transfert de ou vers le buffer de
	l’utilisateur ; on laisse bien entendu le bloc dans le cas pour le cas où il serait accédé
	ultérieurement ;

	— Si l’opération précédente n’est pas possible car le cache est plein (i.e., tous ses blocs
	sont valides), on libère un des blocs du cache pour y copier le bloc disque et donc ainsi
	changer son affectation (le nouveau bloc est alors marqué valide V = 1 et non modifié
	M = 0) ; sélectionner le bloc à libérer est le rôle de l’algorithme de remplacement de
	bloc ; bien entendu, si le bloc à libérer a été modifié pendant sa durée de résidence
	(ou, de manière équivalente, de validité) dans le cache, il faut le réécrire sur disque
	avant de changer son affectation. */
}