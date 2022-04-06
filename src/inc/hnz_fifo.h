//
// Created by Lucas Barret on 31/03/2021.
//

#ifndef WORK_HNZ_FIFO_H
#define WORK_HNZ_FIFO_H
/*! \addtogroup Srv
 * @{
 */
/*! \file /Inc/Srv/ThreadMQ.h \brief Classe de gestion d'une MessageQueue pour la communication inter thread cette classe est basée sur les template afin de faciliter sa réutilisation avec des éléments de types différents */


#include <list>
#include "../hal/inc/CriticalSection.h"
#include <iterator>

//CThreadMQ


template <class T> class CMQ {
public:
    CMQ();
    virtual ~CMQ();


    // Fonctions Gestion MQ
    void	vRaz() ;	// Supression des elements
    virtual T*		pDepile()  ;
    virtual	bool	bEmpile(T* pElt) ;

    virtual		int		iGetNbElt()  { return m_MQ.size() ; } ;
    virtual	void	vDetruit(typename std::list<T*>::iterator pos) ;
    virtual	T*		pCopieSuivant(typename std::list<T*>::iterator &posSuivant, typename std::list<T*>::iterator &posCourant) ;

protected :
    std::list<T*>			m_MQ ;			// Fifo de T*
};

template <class T> class CThreadMQ: public CMQ<T> {
public:
    CThreadMQ();
    virtual ~CThreadMQ();


    // Fonctions Gestion MQ
    virtual	int		iGetNbElt();
    virtual	T*		pDepile()  ;
    virtual	bool 	bEmpile(T* pElt) ;
    virtual	void	vDetruit(typename std::list<T*>::iterator pos) ;
    virtual	T*		pCopieSuivant(typename std::list<T*>::iterator &posSuivant, typename std::list<T*>::iterator &posCourant) ;
    //int		iGetHandle() { return m_EvtMQ.iGetHandle() ; } ;
    //BOOL	bWaitEvtFifo(DWORD dwTimeOut) { return m_EvtMQ.bWaitEvent(dwTimeOut);} ;

protected :
    CCriticalSection	m_csAcces ;		// Section critique pour acces partagé
    //CEvtFifo			m_EvtMQ ;		// Evenement attaché a la fifo
};


//
// Implementation de la classe  CMQ
//

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
template <class T>
CMQ<T>::CMQ(){}

template <class T>
CMQ<T>::~CMQ(){

    // Supression des pointeurs de la liste
    vRaz() ;
}

/*!
********************************************************************************
Raz de la liste
********************************************************************************
--*/
template <class T>
void CMQ<T>::vRaz(){

    T * pElt ;

    // Rez des elements
    while ((pElt = pDepile()) != NULL) {
        delete pElt ;
    }
}

/*!
********************************************************************************
Lecture d'un élément de la liste et repositionnement de l'évènement si la liste
n'est pas vide
********************************************************************************
--*/
template <class T>
T* CMQ<T>::pDepile() {

    T * pElt	= NULL ;

    // Test pile vide
    if (!m_MQ.empty()) {
        // Retire le 1er elt de la liste
        pElt = *m_MQ.begin();
        m_MQ.pop_front() ;
    }

    return pElt ;
}

/*!
********************************************************************************
Ajout d'un élément dans la liste et positionnement de l'évènement si la liste
était vide
********************************************************************************
--*/
template <class T>
bool CMQ<T>::bEmpile(T* pElt){

    // Test ptr non null
    if (pElt == NULL) return false ;

    // Ajout en fond
    m_MQ.insert(m_MQ.end(), pElt) ;

    return true ;
}

/*!
********************************************************************************
Copie d'un élément avec évolution automatique du pointeur
********************************************************************************
--*/
template <class T>
T* CMQ<T>::pCopieSuivant(typename std::list<T*>::iterator &posSuivant, typename std::list<T*>::iterator &posCourant) {
    T * pElt	= NULL ;

    // Test pile vide
    if (!m_MQ.empty()) {
        //if (posSuivant == (typename std::list<T*>::iterator)NULL) posSuivant = m_MQ.begin();

        // Mémorisation de la position courant
        posCourant = posSuivant;

        // Lit l'élément de la liste et incrémente la position
        if (posSuivant != m_MQ.end()) {
            pElt = *posSuivant;
            posSuivant ++;
        }
        //else posSuivant = (typename std::list<T*>::iterator)NULL;
    }

    return pElt ;
}

/*!
********************************************************************************
Suppression d'un élément
********************************************************************************
--*/
template <class T>
void CMQ<T>::vDetruit(typename std::list<T*>::iterator pos) {

    typename std::list<T*>::iterator emptyList;
    *emptyList = NULL;

    if (pos == emptyList) return;

    // Suppresion
    m_MQ.erase(pos);
}


//
// Implementation de la classe  CThreadMQ
//

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
template <class T>
CThreadMQ<T>::CThreadMQ(){}

template <class T>
CThreadMQ<T>::~CThreadMQ(){}

/*!
********************************************************************************
Lecture d'un élément de la liste et repositionnement de l'évènement si la liste
n'est pas vide
********************************************************************************
--*/
template <class T>
T* CThreadMQ<T>::pDepile() {

    T * pElt;

    // Protection pour multithread
    m_csAcces.Lock() ;

    // Retire le 1er elt de la liste
    pElt = CMQ<T>::pDepile();

    //if (CMQ<T>::iGetNbElt() != 0) m_EvtMQ.SetEvent() ;

    // Sortie de la section critique
    m_csAcces.Unlock() ;

    return pElt ;
}

/*!
********************************************************************************
Obtention du nombre d'élements de la file
********************************************************************************
--*/
template <class T>
int CThreadMQ<T>::iGetNbElt(){

    // Protection pour multithread
    m_csAcces.Lock() ;

    // Etait-elle vide ?
    int iNb = CMQ<T>::iGetNbElt();

    // Sortie de la section critique
    m_csAcces.Unlock() ;

    return iNb ;
}

/*!
********************************************************************************
Ajout d'un élément dans la liste et positionnement de l'évènement si la liste
était vide
********************************************************************************
--*/
template <class T>
bool CThreadMQ<T>::bEmpile(T* pElt){
    bool bSet;

    // Test ptr non null
    if (pElt == NULL) return false ;

    // Protection pour multithread
    m_csAcces.Lock() ;

    // Etait-elle vide ?
    bSet = (CMQ<T>::iGetNbElt() == 0);

    // Ajout en fond
    CMQ<T>::bEmpile(pElt);

    // Indique pile non vide ssi elle était vide
   // if (bSet) m_EvtMQ.SetEvent();

    // Sortie de la section critique
    m_csAcces.Unlock() ;

    return true ;
}

/*!
********************************************************************************
Copie d'un élément avec évolution automatique du pointeur
********************************************************************************
--*/
template <class T>
T* CThreadMQ<T>::pCopieSuivant(typename std::list<T*>::iterator &posSuivant, typename std::list<T*>::iterator &posCourant) {
    T * pElt	= NULL ;

    // Protection pour multithread
    m_csAcces.Lock() ;

    // Copie
    pElt = CMQ<T>::pCopieSuivant(posSuivant, posCourant);

    // Sortie de la section critique
    m_csAcces.Unlock() ;

    return pElt ;
}

/*!
********************************************************************************
Suppression d'un élément
********************************************************************************
--*/
template <class T>
void CThreadMQ<T>::vDetruit(typename std::list<T*>::iterator pos) {

    typename std::list<T*>::iterator emptyList;
    *emptyList = NULL;

    if (pos == emptyList) return;

    // Protection pour multithread
    m_csAcces.Lock() ;

    // Suppression
    CMQ<T>::vDetruit(pos);

    //if (CMQ<T>::iGetNbElt() != 0) m_EvtMQ.SetEvent() ;

    // Sortie de la section critique
    m_csAcces.Unlock() ;
}



#endif //WORK_HNZ_FIFO_H
