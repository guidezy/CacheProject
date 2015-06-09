/*!
 * \file NUR_Strategy.c
 *
 * \brief  Stratégie de remplacement d’un bloc non utilisé récemment
 * 
 * \author Corentin HARDY
 *
 * $Id: NUR_strategy.c,v 0.1 2015/06/08 10:24:06 jpr Exp $
 */

#include <assert.h>

#include "strategy.h"
#include "low_cache.h"
#include "random.h"

int count_nderef; //< va de 0 à nderef(du cache). sert à dire quand reinitialiser les bits R.
int deuxR; //< une constante: R + VALUE + MODIF = deuxR - 1.
#ifdef DEBUG
int c1 = 0; int c2 = 0; int c3 = 3; int c4 = 0;
#endif
/*!
 * On reintialise tous bit de reference des blocs, ainsi que le compteur de nderef.
 */
static void initNUR(struct Cache *pcache){
    count_nderef = 0;
    // on reinitialise les bit de references de tous les blocs
    pcache->instrument.n_deref++;
    for (int i = 0; i < pcache->nblocks; ++i)
    {
        if (((pcache->headers)[i].flags % deuxR == (R + VALID + MODIF)) 
            && ((pcache->headers)[i].flags % deuxR != (R + VALID)) ){
            (pcache->headers)[i].flags -= R;
        }
    }
}

/*!
 * Une méthode qui gère le nderef. Tous les nderef lecture ou ecriture de bloc, on
 * doit reinitialiser les bits de réference de tous les blocs.
 */
static void Count_n_de_Ref(struct Cache *pcache, struct Cache_Block_Header *pbh){
    count_nderef++;
    if ((pbh->flags % deuxR != (R + VALID + MODIF)) && (pbh->flags % deuxR != (R + VALID))){
        pbh->flags += R;
    }
    if (count_nderef >= pcache->nderef){
        initNUR(pcache);
    }
}

/*!
 * NRU : on initialise un compteur pour compter jusqu'à nderef.
 */
void *Strategy_Create(struct Cache *pcache) 
{
    count_nderef = 0;
    deuxR = 2 * R;
    return NULL;
}

/*!
 * NRU : Rien à faire ici.
 */
void Strategy_Close(struct Cache *pcache){
    #ifdef DEBUG
    printf("%d, %d, %d, %d.\n", c1, c2, c3, c4);
    #endif
}

/*!
 * NRU : on reinitialise le compteur de nderef.
 */
void Strategy_Invalidate(struct Cache *pcache)
{
    initNUR(pcache);
    #ifdef DEBUG
    printf("%d, %d, %d, %d.\n", c1, c2, c3, c4);
    #endif
}

/*! 
 * NRU : on parcours les blocs, on prend le premier meilleurs bloc, 
 * dans l'ordre de VALID uniquement, puis MODIF, après B. 
 * Sinon on prend un bloc aleatoire.
 */
struct Cache_Block_Header *Strategy_Replace_Block(struct Cache *pcache) 
{
    struct Cache_Block_Header *pbh;
    if ((pbh = Get_Free_Block(pcache)) != NULL){
        return pbh;
    }

    for (int i = 0; i < pcache->nblocks; ++i)
        if ((pcache->headers)[i].flags % deuxR == VALID){
            #ifdef DEBUG
            c1++;
            #endif
            return &pcache->headers[i];
        }

    for (int i = 0; i < pcache->nblocks; ++i)
        if ((pcache->headers)[i].flags % deuxR == (VALID + MODIF)){
            #ifdef DEBUG
            c2++;
            #endif
            return &pcache->headers[i];
        }

    for (int i = 0; i < pcache->nblocks; ++i)
        if ((pcache->headers)[i].flags % deuxR == (VALID + R)){
            #ifdef DEBUG
            c3++;
            #endif
            return &pcache->headers[i];
        }
    #ifdef DEBUG
    c4++;
    #endif
    // si on ne trouve rien, on tire un bloc aleatoire.
    return &pcache->headers[RANDOM(0, pcache->nblocks)];
}

/*!
 * NUR : on compte le nombre de lecture pour le cache en entier.
 * on ajoute le bit de reference au block actuel.
 */
void Strategy_Read(struct Cache *pcache, struct Cache_Block_Header *pbh) 
{
    Count_n_de_Ref(pcache, pbh);
}  

/*!
 * NUR : on compte le nombre d'ecriture pour le cache en entier.
 * on ajoute le bit de reference au block actuel.
 */  
void Strategy_Write(struct Cache *pcache, struct Cache_Block_Header *pbh)
{
    Count_n_de_Ref(pcache, pbh);
} 

char *Strategy_Name()
{
    return "NUR";
}