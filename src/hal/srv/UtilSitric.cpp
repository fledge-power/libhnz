//
// Created by estelle on 21/04/2021.
//

#include "../inc/UtilSitric.h"

/*! \file /Com/Utilitaire/UtilSitric.cpp \brief Fonctions utilitaires SITRIC */


#include "../inc/StdAfx.h"
#include <math.h>
#include "../inc/UtilSitric.h"
#include <limits.h>
#include <float.h>

#include "../inc/CriticalSection.h"
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/times.h>
#include <ctype.h>

using namespace std;

#define BASE_CPT_TICKS		(0x0000000100000000LL * 10LL)

static	uint64_t 	gui64CptTicks   = 0 ;		// Nombre de rotations autour du DWORD
static	uint64_t 	gui64AncienTick = 0 ;
static	CCriticalSection gSectionCritiqueTemps;	///< Section critique pour gestion du temps des timers


/*!
********************************************************************************
\brief Récupération de l'heure actuelle gmt en us
\return Renvoi l'heure gmt actuelle
********************************************************************************
*/
uint64_t ui64GetTimeMS() {

    UINT64 iTps;

    // Sous UNIX utilisation de gettimeof day et conversion dans un 64 bits
    struct timeval ttCourant;
    struct timezone tz;

    gettimeofday(&ttCourant, &tz);
    iTps = ((UINT64) ttCourant.tv_sec * 1000) + ((UINT64) ttCourant.tv_usec / 1000);


    return iTps;
}

/*!
********************************************************************************
\brief Récupération du temps en ms ecoulé depuis la mise en route
\return Renvoi le temps écoulé depuis la mise en route
********************************************************************************
*/
uint64_t ui64GetTimerMS() {

    // Début de section critique
    gSectionCritiqueTemps.Lock();


    // Sous LINUX utilisation times ==> pas de 10ms
    UINT64
    uiTick = (UINT64)(UINT)
    times(NULL);
    // Passage en ms
    uiTick *= 10;

    // test rotation
    if (uiTick < gui64AncienTick) gui64CptTicks += BASE_CPT_TICKS;
    gui64AncienTick = uiTick;

    // Fin de section critique
    gSectionCritiqueTemps.Unlock();


    return gui64CptTicks + uiTick;
}

