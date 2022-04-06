//
// Created by estelle on 04/04/2021.
//

#ifndef PERSO_HNZ_SRV_TCP_H
#define PERSO_HNZ_SRV_TCP_H

#include "SRV_Canal.h"

//
// Map STL pour la gestion des Canaux
//
#include <map>
typedef std::map<int,CSRV_Canal*>	mapSOCK2CANAL ;
typedef std::map<CSRV_Canal*,CSRV_Canal*>	mapCANAL2CANAL ;

extern "C" {
extern void vSRV_GetVersion(char* szVersion , char* szDate) ;
}

#define TCP_MASQUE_LECTURE			0x01
#define TCP_MASQUE_ECRITURE			0x02
#define TCP_MASQUE_EXCEPTION		0x04

#define TCP_CLEAR_MASQUE_LECTURE	0x10
#define TCP_CLEAR_MASQUE_ECRITURE	0x20
#define TCP_CLEAR_MASQUE_EXCEPTION	0x40

#define ERR_LECTURE                 -2


/// Classe implémentant la couche TCP
class CSRV_Tcp  {

// Attributes
public:


    BOOL	m_bTrtStop ;

// Operations
public:

    CSRV_Tcp();
    virtual ~CSRV_Tcp();

    void	vInit() ;
    int		iTraiteCanaux(LONG lTimeOut, CSRV_Canal * pCanal = NULL);
    int		bAjouteCanalService(CSRV_Canal * pCanal, int iMasques=TCP_MASQUE_LECTURE|TCP_MASQUE_EXCEPTION);
    int		bSupprimeCanalService(CSRV_Canal * pCanal);
    int		vInitMasques(CSRV_Canal *pCanal, int iMasques);
    int		vInitMasques(SOCKET s, int iMasques);
    void	vMemoriseCanalPourDestruction(CSRV_Canal  *pCanal);
    void	vEffaceCanalPourDestruction();

    BOOL	bPing(LPCSTR szHostName, DWORD dwTimeout, int iBoucles);
    LPCSTR	szGetError() {	return m_szErreur; }

    static 	ULONG ulGetHostByName(LPCSTR szMachine);

// Overrides
public:

    static int m_iTempoEBADF ;
    static int m_iTempoEINVAL ;

    // Implementation
protected:

    void vTraiteLectures(fd_set * pSet) ;
    void vTraiteEcritures(fd_set * pSet) ;
    void vTraiteExceptions(fd_set * pSet) ;

    void vCalculeTimeout(long& lTimeOut, BOOL& bTimeOut, DWORD dwTimeFin, LONG tMemoTO);


    CSRV_Canal* pGetCanal(int iSocket);
    BOOL bDelCanal(int iSocket);

    USHORT usChecksum(USHORT* pBuffer, int nSize) ;
    char m_szErreur[200];
    CCriticalSection	m_csSockets;  ///< SC d'acces aux sockets

    // Masques pour le select
    fd_set	m_fdEcriture ;
    fd_set	m_fdLecture ;
    fd_set	m_fdException ;

    // Tableau des canaux en cours
    mapSOCK2CANAL	m_mapCanaux ;
    mapCANAL2CANAL	m_mapCanauxPourDestruction;


    // Uitlisé uniquement sous unix
    int		m_iSocketMax ;

    // Flag pour init de la classe
    static  unsigned short	m_usNbInit;

    friend class CSRV_Canal ;
};

#endif //PERSO_HNZ_SRV_TCP_H
