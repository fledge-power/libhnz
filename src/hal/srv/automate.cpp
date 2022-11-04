//
// Created by estelle on 21/04/2021.
//
/*! \addtogroup Utilitaire
 * @{
 */
/*! \file Automate.cpp \brief Classe CAutomate et CEtapeAutomate */
/*!
*******************************************************************************************
\class CAutomate

  Cette classe permet de définir et de gérer des automates à états finis ainsi que des timers
  à la ms.
*******************************************************************************************
*/
#include "../inc/StdAfx.h"
#include "../inc/UtilSitric.h"
#include <algorithm>
#include "../inc/automate.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif



/*!
********************************************************************************
   Nom        : bCompareTimer
   Role       : Fonction pour la gestion  du tri d'un vecteur de confTimer
********************************************************************************
*/
struct compTimer : binary_function<char*, char*, bool> {
    bool operator()(const confTIMER &rpStart, const confTIMER &rpEnd) const
    {return rpStart.uiTemps > rpEnd.uiTemps; }
};


/*!
********************************************************************************
\brief Constructeur
********************************************************************************
*/
CAutomate::CAutomate(){
    vClear() ;
}

/*!
********************************************************************************
\brief Destructeur
********************************************************************************
*/
CAutomate::~CAutomate(){
    vClear() ;
}

/*!
********************************************************************************
\brief Supprime la configuration de l'automate
********************************************************************************
*/
void CAutomate::vClear() {

    // Destruction des etapes
    mapETAPES::iterator it;

    for (it = m_mapEtapes.begin(); it != m_mapEtapes.end(); it++) {
        delete it->second;
    }
    m_mapEtapes.clear();

    m_pEtapeEnCours = NULL ;
    m_iEtape		= -9999 ;
}

/*!
********************************************************************************
\brief Ajoute une étape a l'automate
\param iNo Numéro d'étape
\return Renvoi l'étape rajoutée
********************************************************************************
*/
CEtapeAutomate* CAutomate::pNouvelleEtape(int iNo){

    mapETAPES::iterator it ;

    // Verifie si elle existe deja
    it = m_mapEtapes.find(iNo) ;
    if (it != m_mapEtapes.end()) return NULL ;

    // Insertion
    CEtapeAutomate* pEtape = new CEtapeAutomate(iNo) ;

    m_mapEtapes.insert(mapETAPES::value_type(iNo,pEtape)) ;

    return pEtape ;
}

/*!
********************************************************************************
\brief Mise à jour de l'étape en cours
\param iNo Numéro d'étape à affecter
********************************************************************************
*/
void CAutomate::vSetEtape(int iNo) {

    mapETAPES::iterator it ;

    if ((m_iEtape == iNo) && m_pEtapeEnCours) return ;

    m_iEtape = iNo ;

    // Recherche dans le map
    it = m_mapEtapes.find(iNo) ;
    if (it == m_mapEtapes.end()) return ;

    m_pEtapeEnCours = it->second ;


}


/*!
********************************************************************************
\brief Retourne l'action correspondant à l'évenement iEvt sur l'étape en cours et change d'étape si nécessaire
\param iEvt Numéro d'évènement
\return Renvoi l'action correspondante au numéro d'évènement
********************************************************************************
*/
int CAutomate::iGereAutomate(int iEvt){

    if (!m_pEtapeEnCours) return AUTOMATE_ACTION_ERR ;

    // Gestion de la transition
    mapTRANS::iterator it ;

    it = m_pEtapeEnCours->m_mapTransitions.find(iEvt) ;

    // On prend la transition par defaut
    if (it == m_pEtapeEnCours->m_mapTransitions.end()) {
        vSetEtape(m_pEtapeEnCours->m_DefTrans.iEtape) ;
        return m_pEtapeEnCours->m_DefTrans.iAction;
    }

    vSetEtape(it->second.iEtape) ;
    return it->second.iAction;
}


/*!
********************************************************************************
\brief Ajoute un timer à la liste des timers gérés
\param iDuree Durée du timer
\param iAction Action
\param pv
********************************************************************************
*/
void CAutomate::vAddTimer(int iDuree , int iAction , void* pv) {

    confTIMER	conf ;
    conf.iAction  = iAction ;
    conf.pv		  = pv ;
    conf.iDuree	  = iDuree;
    conf.uiTemps  = ui64GetTimerMS() + iDuree ;

    m_aTimers.push_back(conf) ;

    // Tri par priorite le + petit au fond
    std::sort(m_aTimers.begin(),m_aTimers.end(),compTimer()) ;

}

/*!
********************************************************************************
\brief Rearme le timer défini par le couple action/pv
\param iAction Action
\param pv
********************************************************************************
*/
void CAutomate::vResetTimer(int iAction , void* pv) {

    if (m_aTimers.empty()) return ;

    uint64_t uiNow = ui64GetTimerMS();

    // Recherche tous les timers iAction,pv
    for (int i =0 ; i < (int)m_aTimers.size() ; i++) {
        if ((m_aTimers[i].iAction == iAction) &&
            (m_aTimers[i].pv		 == pv )) m_aTimers[i].uiTemps = uiNow + m_aTimers[i].iDuree ;
    }

    // Tri par priorite le + petit au fond
    std::sort(m_aTimers.begin(),m_aTimers.end(),compTimer()) ;


}

/*!
********************************************************************************
\brief Supprime un timer définit par le couple Action/pv de la liste
\param iAction Action
\param pv
********************************************************************************
*/
void CAutomate::vKillTimer(int iAction , void* pv) {

    if (m_aTimers.empty()) return ;

    // Recherche tous les timers iAction,pv les marque à 0
    for (int i =0 ; i < (int)m_aTimers.size() ; i++) {
        if ((m_aTimers[i].iAction == iAction) &&
            (m_aTimers[i].pv		 == pv )) m_aTimers[i].uiTemps = 0 ;
    }

    // Tri par priorite le + petit au fond
    std::sort(m_aTimers.begin(),m_aTimers.end(),compTimer()) ;


    // Kill des timers
    while (!m_aTimers.empty()) {
        if (m_aTimers.back().uiTemps == 0) m_aTimers.pop_back() ;
        else
            return ;
    }

}



/*!
********************************************************************************
\brief Récupération du temps min jusqu'au prochain timer (en ms)
\return Retourne le temps
********************************************************************************
*/
uint CAutomate::uiGetTimeOut() {

    if (m_aTimers.empty()) return 0x7FFFFFF ;

    uint64_t uiNow = ui64GetTimerMS();

    uint64_t uiTps = m_aTimers.back().uiTemps;
    if ( uiTps <= uiNow) return 0  ;

    return uiTps-uiNow  ;
}




/*!
********************************************************************************
\brief Renvoi la prochaine action timer à faire
\param iAction (OUT) Prochaine action
\param pv (OUT)
\return Renvoi vrai en cas de succès, renvoi faux sinon
********************************************************************************
*/
bool CAutomate::bGetNextTimer(int & iAction, void*&pv) {

    if (m_aTimers.empty()) return false;

    uint64_t uiNow = ui64GetTimerMS();

    if ( m_aTimers.back().uiTemps <= uiNow) {

        iAction = m_aTimers.back().iAction ;
        pv		= m_aTimers.back().pv ;
        m_aTimers.pop_back() ;
        return true ;
    }
    return false ;
}


/*!
********************************************************************************
\brief Renvoi le nombre de timers déclarés
********************************************************************************
*/
int CAutomate::iGetNbTimers() {

    return m_aTimers.size() ;
}


/*!
********************************************************************************
\class CEtapeAutomate
Classe de gestion des étapes de l'automate
********************************************************************************
*/


/*!
********************************************************************************
\brief Constructeur
\param iNo Numéro d'étape
********************************************************************************
*/
CEtapeAutomate::CEtapeAutomate(int iNo){
    m_DefTrans.iEtape = iNo ;
    m_DefTrans.iAction= AUTOMATE_NO_ACTION;

}

/*!
********************************************************************************
\brief Destructeur
********************************************************************************
*/
CEtapeAutomate::~CEtapeAutomate(){
}


/*!
********************************************************************************
\brief Ajoute une transition
\param iEvt Numéro d'évènement
\param iEtape Numéro d'étape
\param iAction Action associée
\return Renvoi vrai en cas de succès, renvoi faux sinon
********************************************************************************
*/
bool CEtapeAutomate::bAddTransitions(int iEvt,int iEtape,int iAction) {

    mapTRANS::iterator it ;

    // Verifie si elle existe deja
    it = m_mapTransitions.find(iEvt) ;
    if (it != m_mapTransitions.end()) return false ;

    // Insertion
    CELLULE_AUTOMATE cell;

    cell.iAction = iAction ;
    cell.iEtape  = iEtape ;

    m_mapTransitions.insert(mapTRANS::value_type(iEvt,cell)) ;

    return true ;
}

/*!
********************************************************************************
\brief Mise a jour du comportement par defaut
\param iEtape Numéro d'étape
\param iAction Numéro d'action
********************************************************************************
*/
void CEtapeAutomate::vInitDefTrans(int iEtape,int iAction) {
    m_DefTrans.iAction = iAction ;
    m_DefTrans.iEtape  = iEtape ;
}




/** @}*/
