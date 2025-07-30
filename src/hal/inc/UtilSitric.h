//
// Created by estelle on 21/04/2021.
//

#ifndef PERSO_HNZ_UTILSITRIC_H
#define PERSO_HNZ_UTILSITRIC_H
/*! \file /Inc/Utilitaire/UtilSitric.h \brief Définition de fonctions utilitaires */




#include <stdio.h>
#include <stdlib.h>

//#include "Inc/Utilitaire/GestionHeure.h"

#include <algorithm>
#include <list>
#include <string>
#include <map>
#include <vector>
#include <cstdint>
using namespace std;
// Fin STL

#include <time.h>
#include <sys/timeb.h>



uint64_t 	ui64GetTimeMS() ;
uint64_t 	ui64GetTimerMS() ;

#endif //PERSO_HNZ_UTILSITRIC_H
