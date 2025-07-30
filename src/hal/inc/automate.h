//
// Created by estelle on 21/04/2021.
//

#ifndef PERSO_HNZ_AUTOMATE_H
#define PERSO_HNZ_AUTOMATE_H
/*! \file /Inc/Utilitaire/Automate.h \brief Classe de gestion d'un automate à état fini */


#include <stdio.h>
#include <stdlib.h>

#include <vector>
#include <map>
#include <mutex>
#include <cstdint>

// Action par defaut on ne fait rien
#define	AUTOMATE_ACTION_ERR		-1
#define	AUTOMATE_NO_ACTION		 0

class CAutomate ;

typedef struct {
    int		iEtape ;
    int		iAction;
} CELLULE_AUTOMATE;

// Tables des transitions
typedef std::map<int,CELLULE_AUTOMATE>	mapTRANS ;


// Timers
typedef struct {
    uint64_t uiTemps ;
    int		iAction ;
    int		iDuree  ;
    void*	pv ;
} confTIMER ;

typedef std::vector<confTIMER>	aTIMERS ;

/// Classe representant une etape avec ses transitions/actions
class CEtapeAutomate {

public :
    CEtapeAutomate(int iNo);
    virtual ~CEtapeAutomate();

    void	vInitDefTrans (int iEtape,int iAction) ;
    bool	bAddTransitions(int iEvt,int iEtape,int iAction) ;

protected:

    CELLULE_AUTOMATE	m_DefTrans ;
    mapTRANS			m_mapTransitions ;

    friend class CAutomate ;

} ;

// Tableau d'etapes
typedef std::map<int,CEtapeAutomate*>	mapETAPES ;

/// Classe représentant un automate
class CAutomate {

public:

    CAutomate();
    virtual ~CAutomate();

    CEtapeAutomate*	pNouvelleEtape(int iNo) ;
    void			vSetEtape(int iNo) ;
    int				iGetEtape() { return m_iEtape ; } ;
    int				iGereAutomate(int iEvt) ;
    void			vClear() ;

    // Gestion des timers
    void			vAddTimer  (int iDuree , int iAction , void* pv) ;
    void			vResetTimer(int iAction, void* pv) ;
    void			vKillTimer (int iAction, void* pv) ;
    unsigned int 	uiGetTimeOut();
    bool			bGetNextTimer(int & iAction, void*&pv) ;
    void			vRazTimers()  { std::lock_guard<std::mutex> lock{m_timerMutex}; m_aTimers.clear();} ;
    int				iGetNbTimers()  ;


protected :

    int				m_iEtape ;
    mapETAPES		m_mapEtapes ;
    CEtapeAutomate* m_pEtapeEnCours ;
    aTIMERS			m_aTimers ;
    std::mutex      m_timerMutex ;

};


#endif //PERSO_HNZ_AUTOMATE_H
