/*! \addtogroup Utilitaire
 * @{
 */
/*! \file /Com/Utilitaire/CriticalSection.cpp \brief Implémentation de la classe CCriticalSection */
/*!
*******************************************************************
\class CCriticalSection
Classe de getion des sections critiques
*******************************************************************
*/

#include "../inc/CriticalSection.h"


/*!
*******************************************************************
\brief Constructeur
*******************************************************************
*/
CCriticalSection::CCriticalSection()
{
    // Init du MUTEX

    pthread_mutexattr_t oAttrMutex;
    pthread_mutexattr_init(&oAttrMutex);

    //oAttrMutex.__mutexkind = PTHREAD_MUTEX_RECURSIVE_NP ;
    pthread_mutexattr_settype(&oAttrMutex,PTHREAD_MUTEX_RECURSIVE);


    pthread_mutex_init(&m_MUTEX, &oAttrMutex);

}

/*!
*******************************************************************
\brief Destructeur
*******************************************************************
*/
CCriticalSection::~CCriticalSection()
{
    //
    pthread_mutex_destroy(&m_MUTEX);
}

/*!
*******************************************************************
\brief Vérouillage du mutex
*******************************************************************
*/
void
CCriticalSection::Lock()
{
    //
    pthread_mutex_lock(&m_MUTEX);
}

/*!
*******************************************************************
\brief Dévérouillage du mutex
*******************************************************************
*/
void
CCriticalSection::Unlock()
{
    //
    pthread_mutex_unlock(&m_MUTEX);
}




/** @}*/
