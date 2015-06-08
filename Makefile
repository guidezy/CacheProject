#------------------------------------------------------------------
# Un exemple d'implémentation du cache d'un fichier afin d'explorer
# l'effet des algortihmes de gestion et de remplacement
#------------------------------------------------------------------
# Jean-Paul Rigault --- ESSI --- Janvier 2005
#------------------------------------------------------------------
# Makefile
#------------------------------------------------------------------

# Sources

HDR = $(wildcard *.h)
SRC = $(wildcard *.c)
SRC_DIR = src
BIN_DIR = bin
OBJ_DIR = obj

# Exécutables à construire

PROGS = $(BIN_DIR)/tst_Cache_RAND $(BIN_DIR)/tst_Cache_FIFO $(BIN_DIR)/tst_Cache_LRU $(BIN_DIR)/tst_Cache_NUR

# Fichiers de bibliothèque à reconstruire : initialement vide. 
# Mettre ici les *.o de la bibliothèque que vous avez réimplémentés
# (cache.o low_cache.o cache_list.o)

USRFILES = 

#------------------------------------------------------------------
# Commandes
#------------------------------------------------------------------

# Compilateur et options
CC = gcc
CFLAGS = -std=c99 -Wall -g -D_POSIX_C_SOURCE=200809L
MKDEPEND = $(CC) $(CFLAGS) -MM

# Documentation
DOXYGEN = doxygen

#------------------------------------------------------------------
# Règles par défaut
#------------------------------------------------------------------

# Exécutables avec diverses stratégies
$(BIN_DIR)/tst_Cache_% : $(OBJ_DIR)/tst_Cache.o $(OBJ_DIR)/%_strategy.o $(USRFILES)	
	$(CC) -o $@ $^ libCache.a

# Exécution des simulations (make simul) avec paramètres par défaut
$(BIN_DIR)/%_default.out : $(BIN_DIR)/tst_Cache_%
	$< > $@

#------------------------------------------------------------------
# Cibles principales
#------------------------------------------------------------------

# Exécutables : décommentés les exécutables des stratégies que vous avez implémentées
# N'enlevez pas depend !

all : $(BIN_DIR)/depend $(BIN_DIR)/tst_Cache_RAND # $(BIN_DIR)/tst_Cache_FIFO $(BIN_DIR)/tst_Cache_LRU $(BIN_DIR)/tst_Cache_NUR

# Nettoyage 
clean : all
	-rm -f $(OBJ_DIR)/*.o $(BIN_DIR)/*.out foo

# Nettoyage complet
full_clean :
	-rm -f $(OBJ_DIR)/*.o $(BIN_DIR)/*.out foo
	-rm -f $(BIN_DIR)/tst_Cache_RAND $(BIN_DIR)/tst_Cache_FIFO $(BIN_DIR)/tst_Cache_LRU $(BIN_DIR)/tst_Cache_NUR
	-rm $(BIN_DIR)/depend.out
	-rm -rf Plots

#------------------------------------------------------------------
# Simulation du cache et tracé des courbes
#------------------------------------------------------------------

# Génération des courbes dans le répertoire Plots (voir Makefile.plots)
plots : all
	-mkdir Plots
	cp Makefile.plots Plots/Makefile
	cd Plots; make

#------------------------------------------------------------------
# Reconstruction automatique des dépendances
#------------------------------------------------------------------

$(BIN_DIR)/depend : 
	$(MAKE) $(BIN_DIR)/depend.out

$(BIN_DIR)/depend.out : $(SRC_DIR)/$(SRC) $(SRC_DIR)/$(HDR)
	$(MKDEPEND) $(SRC_DIR)/$(SRC) > $(BIN_DIR)/depend.out

include bin/depend.out
