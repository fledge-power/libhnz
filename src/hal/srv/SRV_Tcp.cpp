//
// Created by estelle on 04/04/2021.
//

#include "../inc/SRV_Tcp.h"


/*! \addtogroup Srv
 * @{
 */
/*! \file /Com/Srv/SRV_Tcp.cpp \brief Implementation de la classe CSRV_Tcp*/
/*!
*******************************************************************************************
\class CSRV_Tcp
Classe implémentant la couche TCP
*******************************************************************************************
*/



//#include "Inc/Versions/VersionNOYAU.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define GetErreur() errno
#include <arpa/inet.h>


// Temporisations
int CSRV_Tcp::m_iTempoEBADF = 500;
int CSRV_Tcp::m_iTempoEINVAL = 500;


// Compteur du nombre d'instances de la classe
USHORT CSRV_Tcp::m_usNbInit = 0 ;

extern "C" {
/*!
********************************************************************************
\brief Obtention de la version et dernière date de mise à jour
\param szVersion (OUT) Version
\param szDate (OUT) Date
********************************************************************************
*/
void vSRV_GetVersion(char* szVersion , char* szDate) {
    strcpy(szVersion,VERSION_MODULE) ;
    strcpy(szDate,DATE_MODULE) ;
}
}

//
/*!
*******************************************************************************
\brief Constructeur
********************************************************************************
*/
CSRV_Tcp::CSRV_Tcp(){
    vInit() ;
    m_iSocketMax = 0 ;

    // Pour le ping
    m_szErreur[0]=0;
}


/*!
*******************************************************************************
\brief Init de WinSock et des variables de la classe
********************************************************************************
*/
void CSRV_Tcp::vInit() {

    // Pas de canaux en cours
    FD_ZERO(&m_fdLecture) ;
    FD_ZERO(&m_fdEcriture) ;
    FD_ZERO(&m_fdException) ;

    m_usNbInit++ ;
}

/*!
*******************************************************************************
\brief Calcul du checksum pour le ping
\param pBuffer Buffer
\param nSize Taille du buffer
\return Renvoi le checksum
********************************************************************************
*/
USHORT CSRV_Tcp::usChecksum(USHORT* pBuffer, int nSize) {
    unsigned long nCheckSum = 0;

    while(nSize >1)
    {
        nCheckSum += *pBuffer++;
        nSize	  -=sizeof(USHORT);
    }

    if(nSize)
        nCheckSum += *(UCHAR*)pBuffer;

    nCheckSum = (nCheckSum >> 16) + (nCheckSum & 0xffff);
    nCheckSum += (nCheckSum >>16);

    return (USHORT)(~nCheckSum);
}

/////////////////////////////////////////////////////////////////////////////
// IpHeader

typedef struct IpHeaderTag
{
    unsigned int h_len:4;          // length of the header
    unsigned int version:4;        // Version of IP
    unsigned char tos;             // Type of service
    unsigned short total_len;      // total length of the packet
    unsigned short ident;          // unique identifier
    unsigned short frag_and_flags; // flags
    unsigned char ttl;
    unsigned char proto;           // protocol (TCP, UDP etc)
    unsigned short checksum;       // IP checksum
    unsigned int sourceIP;
    unsigned int destIP;
}
        IpHeader;

/////////////////////////////////////////////////////////////////////////////
// IcmpHeader

typedef struct IcmpHeaderTag
{
    BYTE i_type;
    BYTE i_code;				   // type sub code
    USHORT i_cksum;
    USHORT i_id;
    USHORT i_seq;
}
        IcmpHeader;


/*!
********************************************************************************
\brief Destructeur : Deconnexion de WinSock
********************************************************************************
*/
CSRV_Tcp::~CSRV_Tcp(){

    m_usNbInit -- ;

    // Raz du map ( c'est uniquement ici que l'on delete les reperes )
    mapSOCK2CANAL::iterator it;
    CSRV_Canal  *pCa ;

    for (it = m_mapCanaux.begin(); it != m_mapCanaux.end(); it++) {
        // On vide l'élément s'il est en DeleteAuto
        pCa = it->second ;
        pCa->m_pInterfaceTcp = NULL ; // Au cas ou l'objet est detruit plus tard (gerreurlog!)
        if (pCa->m_bDeleteAuto)	delete pCa;
    }
    m_mapCanaux.clear();

    vEffaceCanalPourDestruction();
}

/*!
********************************************************************************
\brief Ajout d'un canal dans la table associative des canaux
\param pCanal Canal à ajouter
\param iMasques Masques
\return Renvoi vrai en cas de succès, renvoi faux sinon
********************************************************************************

int CSRV_Tcp::bAjouteCanalService(CSRV_Canal * pCanal, int iMasques){

    ASSERT(pCanal != NULL) ;
    if (pCanal->s == SOCKET_ERROR) {
        ERREUR(MASQUE_ERREUR,"Appel bAjouteCanalService avec socket non définie") ;
        return false ;
    }

    // Recherche dans le map
    CCSLock monLock(&m_csSockets) ;

    mapSOCK2CANAL::iterator it;

    // Recherche si le no de socket existe deja dans le map
    it = m_mapCanaux.find(pCanal->s) ;
    if (it != m_mapCanaux.end()) {
        ERREUR(MASQUE_ERREUR_CRITIQUE,"Double utilisation du socket %d",pCanal->s) ;
        return false ;
    }

    // Insertion dans le map
    m_mapCanaux.insert(mapSOCK2CANAL::value_type(pCanal->s, pCanal)) ;
    if ( pCanal->s > m_iSocketMax )  m_iSocketMax = pCanal->s ;

    // Ajout aux masques du select
    if (iMasques & TCP_MASQUE_LECTURE)		FD_SET(pCanal->s,&m_fdLecture) ;
    if (iMasques & TCP_MASQUE_ECRITURE)		FD_SET(pCanal->s,&m_fdEcriture) ;
    if (iMasques & TCP_MASQUE_EXCEPTION)	FD_SET(pCanal->s,&m_fdException) ;

    return true ;
}*/

/*!
********************************************************************************
\brief Mise à jour des masques protégés par une section critique
\param pCanal Canal
\param iMasques Masques
\return Renvoi vrai
********************************************************************************
*/
int CSRV_Tcp::vInitMasques(CSRV_Canal *pCanal, int iMasques){

    ASSERT(pCanal != NULL) ;
    return vInitMasques(pCanal->s,iMasques) ;
}

/*!
********************************************************************************
\brief Mise à jour des masques protégée par une section critique
\param s Socket
\param iMasques Masques
\return Renvoi vrai
********************************************************************************
*/
int	CSRV_Tcp::vInitMasques(SOCKET s, int iMasques) {

    // Protection
    CCSLock monLock(&m_csSockets) ;
    if (s == SOCKET_ERROR) {
        ERREUR(MASQUE_ERREUR,"Appel vInitMasques avec socket non définie") ;
        return false ;
    }

    // Efface aux masques du select
    if (iMasques & TCP_CLEAR_MASQUE_LECTURE)		FD_CLR(s,&m_fdLecture) ;
    if (iMasques & TCP_CLEAR_MASQUE_ECRITURE)		FD_CLR(s,&m_fdEcriture) ;
    if (iMasques & TCP_CLEAR_MASQUE_EXCEPTION)		FD_CLR(s,&m_fdException) ;


    // Ajout aux masques du select
    if (iMasques & TCP_MASQUE_LECTURE)		FD_SET(s,&m_fdLecture) ;
    if (iMasques & TCP_MASQUE_ECRITURE)		FD_SET(s,&m_fdEcriture) ;
    if (iMasques & TCP_MASQUE_EXCEPTION)	FD_SET(s,&m_fdException) ;

    return true ;
}

/*!
********************************************************************************
\brief Suppression d'un canal dans la table associative des canaux
\param pCanal Canal à supprimer
\return Renvoi vrai
********************************************************************************
*/
int CSRV_Tcp::bSupprimeCanalService(CSRV_Canal * pCanal){

    ASSERT(pCanal != NULL) ;

    CCSLock monLock(&m_csSockets) ;

    if (pCanal->s == SOCKET_ERROR) {
        return true;
    }
    // Suppression du canal du map
    m_mapCanaux.erase(pCanal->s) ;

    // Suppression des masques du select
    FD_CLR(pCanal->s,&m_fdLecture) ;
    FD_CLR(pCanal->s,&m_fdEcriture) ;
    FD_CLR(pCanal->s,&m_fdException) ;

    return true ;
}

/*!
********************************************************************************
\brief Teste les infos recues sur les sockets
\param lTimeOut Time Out
\param pCanal Canal
\return Renvoi 0 en cas de succès, < 0 sinon
********************************************************************************
*/
int CSRV_Tcp::iTraiteCanaux(LONG lTimeOut, CSRV_Canal * pCanal){

    fd_set	fdRetourLecture ;
    fd_set	fdRetourEcriture ;
    fd_set	fdRetourException;
    int		iErr ;
    int     iNb ;
    BOOL	bTimeOut ;
    LONG	lMemoTO  = lTimeOut;
    int		iNbEINVAL = 0;
    int		iNbEBADF = 0;
    BOOL	bErrEBADF;
    BOOL	bErrEINVAL;

// Time out pour select
    struct timeval tvTo ;


    // Vérifie qu'il existe des canaux à traiter
    m_csSockets.Lock();
    if (m_mapCanaux.empty()) {
        m_csSockets.Unlock();
        return -1 ;
    }
    m_csSockets.Unlock();

    // Calcul du timeout final en ms

    struct timeval ttCourant;
    struct timezone tz;

    gettimeofday(&ttCourant, &tz);
    DWORD dwTimeFin = 	(ttCourant.tv_sec * 1000) +
                         (ttCourant.tv_usec / 1000) +
                         lTimeOut;

    do {

        m_bTrtStop = false ;
        bTimeOut = false ;

        // Calcul du time out
        tvTo.tv_sec =  lTimeOut / 1000 ;
        tvTo.tv_usec = (lTimeOut % 1000) * 1000 ;

        // Sauvegarde des masques
        if (pCanal == NULL) {
            // Avant de prendre les masques potentiels, on détruit les contextes en attente de destruction
            vEffaceCanalPourDestruction();
            m_csSockets.Lock();
            memcpy(&fdRetourLecture,&m_fdLecture,sizeof(m_fdLecture)) ;
            memcpy(&fdRetourEcriture,&m_fdEcriture,sizeof(m_fdEcriture)) ;
            memcpy(&fdRetourException,&m_fdException,sizeof(m_fdException)) ;
            m_csSockets.Unlock();
        }
        else {
            FD_ZERO(&fdRetourLecture) ;
            FD_ZERO(&fdRetourEcriture) ;
            FD_ZERO(&fdRetourException) ;
            // Ajout aux masques du select du seul canal voulu
            FD_SET(pCanal->s,&fdRetourLecture) ;
            FD_SET(pCanal->s,&fdRetourException) ;
        }

        // Test des sockets

#ifdef _USEPOLL

        //--- PS AJOUT creation vars pour utilisation de poll a la place de select
		// Creation Variables pour le poll
		struct	pollfd afd[FD_SETSIZE] ;
		int		iNbPoll = 0 ;

		for ( int i = 0 ; i <= m_iSocketMax ; i++) {
			if (!FD_ISSET(i,&fdRetourLecture)) continue ;
			afd[iNbPoll].fd		= i ;
			afd[iNbPoll].events	= POLLIN | POLLERR;
			if (FD_ISSET(i,&fdRetourEcriture)) afd[iNbPoll].events	= POLLOUT ;
			iNbPoll++;
		}


		int iNb = poll(afd, iNbPoll, lTimeOut);

		// Mise a jour des fd de retour
		if (iNb > 0 ) {

			FD_ZERO(&fdRetourLecture) ;
			FD_ZERO(&fdRetourEcriture) ;
			FD_ZERO(&fdRetourException) ;

			for ( int i = 0 ; i < iNbPoll ; i++) {
				if (afd[i].revents & POLLIN ) FD_SET(afd[i].fd,&fdRetourLecture) ;
				if (afd[i].revents & POLLOUT) FD_SET(afd[i].fd,&fdRetourEcriture) ;
				if (afd[i].revents & POLLERR) FD_SET(afd[i].fd,&fdRetourException) ;
			}
		}
#else
        iNb = select(FD_SETSIZE,&fdRetourLecture,&fdRetourEcriture,&fdRetourException,&tvTo) ;

#endif // _USEPOLL


        if (iNb == SOCKET_ERROR) {
            iErr = errno;
            if (errno == EINTR) continue ;		// Cas sigterm ... sous unix
            bErrEINVAL = (iErr == EINVAL);
            bErrEBADF = (iErr == EBADF);


            // Il est possible d'avoir des erreurs EBADF si un des sockets s'est fermé avant le sélect
            // On considère qu'il y a un problème après 5 EBADF consécutifs
            if (bErrEBADF) {
                if (iNbEBADF++ < 5) {
                    ERREUR(MASQUE_TRACE_IMPORTANTE, "EBADF sur select nb=%d", iNbEBADF);
                    Sleep(m_iTempoEBADF);
                    vCalculeTimeout(lTimeOut, bTimeOut, dwTimeFin, lMemoTO);
                    continue;
                }
                else {
                    ERREUR(MASQUE_ERREUR_CRITIQUE,"EBADF: Erreur sur select %d (to.sec = %d to.usec=%d)",iErr, (int) tvTo.tv_sec,(int) tvTo.tv_usec)  ;
                    return ERR_LECTURE;
                }
            }
            else iNbEBADF = 0;
            // Idem pour les messages EINVAL
            if (bErrEINVAL) {
                if (iNbEINVAL++ < 5) {
                    ERREUR(MASQUE_TRACE_IMPORTANTE, "EINVAL sur select nb=%d", iNbEINVAL);
                    Sleep(m_iTempoEINVAL);
                    vCalculeTimeout(lTimeOut, bTimeOut, dwTimeFin, lMemoTO);
                    continue;
                }
                else {
                    ERREUR(MASQUE_ERREUR_CRITIQUE,"EINVAL: Erreur sur select %d (to.sec = %d to.usec=%d)",iErr,(int) tvTo.tv_sec,(int) tvTo.tv_usec)  ;
                    return ERR_LECTURE;
                }
            }
            else iNbEINVAL = 0;

            ERREUR(MASQUE_ERREUR_CRITIQUE,"Erreur sur select %d (to.sec = %d to.usec=%d)",iErr,(int)tvTo.tv_sec,(int)tvTo.tv_usec)  ;
            return ERR_LECTURE ;
        } else {
            iNbEBADF = 0;
            iNbEINVAL = 0;
        }

        // Traitement des canaux
        if (iNb != 0 ) {
            vTraiteLectures(&fdRetourLecture) ;
            vTraiteEcritures(&fdRetourEcriture) ;
            vTraiteExceptions(&fdRetourException) ;
        }
        // Calcul prochain timeout
        vCalculeTimeout(lTimeOut, bTimeOut, dwTimeFin, lMemoTO);
    } while (!m_bTrtStop && !bTimeOut) ;
    return 0 ;
}

/**
******************************************************************************
* @brief Calcule si la fonction iTraiteCanaux est en timeout
* \param lTimeOut (out) Valeur du timeout
* \param bTimeOut (out) Indique si le timeout a été atteint*
******************************************************************************
*/
void CSRV_Tcp::vCalculeTimeout(long& lTimeOut, BOOL& bTimeOut, DWORD dwTimeFin, LONG lMemoTO) {
    struct timeval ttCourant;
    struct timezone tz;

    // Calcul prochain timeout

    gettimeofday(&ttCourant, &tz);
    lTimeOut = dwTimeFin - (	(ttCourant.tv_sec * 1000) +
                                (ttCourant.tv_usec / 1000) );

    if ((lTimeOut <= 0) || (lTimeOut > lMemoTO))  bTimeOut = true ;
}

/*!
********************************************************************************
\brief Recherche du canal correspondant au socket dans le MAP
\param iSocket Numéro de socket
\return Renvoi le canal correspondant
********************************************************************************
*/
CSRV_Canal *CSRV_Tcp::pGetCanal(int iSocket){
    mapSOCK2CANAL::iterator it;
    CSRV_Canal  *pCa = NULL;

    // Recherche dans le map
    CCSLock monLock(&m_csSockets) ;

    it = m_mapCanaux.find(iSocket) ;
    if (it != m_mapCanaux.end()) pCa = it->second ;

    return pCa;
}

/*!
********************************************************************************
\brief Retrait du canal correspondant au socket dans le MAP
\param iSocket Numéro de socket
\return Renvoi vrai
********************************************************************************
*/
BOOL CSRV_Tcp::bDelCanal(int iSocket){

    // Recherche dans le map
    CCSLock monLock(&m_csSockets) ;

    m_mapCanaux.erase(iSocket) ;

    return true;
}

/*!
********************************************************************************
\brief Efface les canaux marqués pour destruction ultérieure
********************************************************************************
*/
void CSRV_Tcp::vEffaceCanalPourDestruction(){
    // Recherche dans le map
    CCSLock monLock(&m_csSockets) ;

    mapCANAL2CANAL::iterator it2;
    CSRV_Canal  *pCa ;
    for (it2 = m_mapCanauxPourDestruction.begin(); it2 != m_mapCanauxPourDestruction.end(); it2++) {
        // On vide l'élément s'il est en DeleteAuto
        pCa = it2->second ;
        pCa->m_pInterfaceTcp = NULL ; // Au cas ou l'objet est detruit plus tard (gerreurlog!)
        ERREUR(MASQUE_DEBUG,"Destruction effective du canal [%p]",pCa) ;
        delete pCa;
    }
    m_mapCanauxPourDestruction.clear();
}

/*!
********************************************************************************
\brief Mémorise les canaux pour destruction ultérieure
\param pCanal Canal à mémoriser
********************************************************************************
*/
void CSRV_Tcp::vMemoriseCanalPourDestruction(CSRV_Canal  *pCanal){

    CCSLock monLock(&m_csSockets) ;

    mapCANAL2CANAL::iterator it;

    // Recherche si le no de socket existe deja dans le map
    it = m_mapCanauxPourDestruction.find(pCanal) ;
    if (it != m_mapCanauxPourDestruction.end()) {
        ERREUR(MASQUE_ERREUR_CRITIQUE,"Ce canal [%p] est déjà marqué à détruire",pCanal) ;
        return ;
    }
    ERREUR(MASQUE_DEBUG,"Marquage du canal [%p] à détruire",pCanal) ;

    // Insertion dans le map
    m_mapCanauxPourDestruction.insert(mapCANAL2CANAL::value_type(pCanal, pCanal)) ;
}

/*!
********************************************************************************
\brief Appelle le traitement a effectuer canal / canal pour les lectures
\param pSet Descripteur de socket
********************************************************************************
*/
void CSRV_Tcp::vTraiteLectures(fd_set * pSet){

    CSRV_Canal  *pCa ;
    int			iSocket ;

    for ( int i = 0 ; i <= m_iSocketMax ; i++) {

        // Test socket claqué
        if (!FD_ISSET(i,pSet)) continue ;
        iSocket = i ;

        pCa = pGetCanal(iSocket);
        if (pCa) {

            pCa->iTraiteLecture() ;

            // Traitement demande de stop de l'utilisateur
            if (pCa->m_bStop) {
                m_bTrtStop = true ;
                return ;
            }

            // Traitement fermeture canal ==> suppression canal
            if (pCa->s == SOCKET_ERROR) {
                bDelCanal(iSocket) ;
                if (pCa->m_bDeleteAuto) {
                    delete pCa ;
                }
            }
        }
        else {
            ERREUR(MASQUE_ERREUR,"LECT:Canal non défini pour socket %d",iSocket) ;
            vInitMasques(iSocket,TCP_CLEAR_MASQUE_LECTURE|TCP_CLEAR_MASQUE_ECRITURE|TCP_CLEAR_MASQUE_EXCEPTION) ;
        }
    }
    return ;
}

/*!
********************************************************************************
\brief Appelle le traitement à effectuer canal / canal pour les écritures
\param pSet Descripteur de socket
********************************************************************************
*/
void CSRV_Tcp::vTraiteEcritures(fd_set * pSet){

    CSRV_Canal  *pCa ;
    int			iSocket ;

    for ( int i = 0 ; i <= m_iSocketMax ; i++) {
        // Test socket claqué
        if (!FD_ISSET(i,pSet)) continue ;
        iSocket = i ;

        pCa = pGetCanal(iSocket);
        if (pCa) {

            pCa->vCSEcriture(true) ;
            pCa->iTraiteEcriture() ;
            pCa->vCSEcriture(false) ;

            // Traitement demande de stop de l'utilisateur
            if (pCa->m_bStop) {
                m_bTrtStop = true ;
                return ;
            }

            // Traitement fermeture canal ==> suppression canal
            if (pCa->s == SOCKET_ERROR) {
                bDelCanal(iSocket) ;
                if (pCa->m_bDeleteAuto) {
                    delete pCa ;
                }
            }
        }
        else {
            ERREUR(MASQUE_ERREUR,"ECRI:Canal non défini pour socket %d",iSocket) ;
            vInitMasques(iSocket,TCP_CLEAR_MASQUE_ECRITURE) ;
        }
    }
    return ;
}

/*!
********************************************************************************
\brief Appelle le traitement a effectuer canal / canal pour les exceptions
\param pSet Descripteur de socket
********************************************************************************
*/
void CSRV_Tcp::vTraiteExceptions(fd_set * pSet){

    CSRV_Canal  *pCa ;
    int			iSocket ;

    for ( int i = 0 ; i <= m_iSocketMax ; i++) {
        // Test socket claqué
        if (!FD_ISSET(i,pSet)) continue ;
        iSocket = i;

        pCa = pGetCanal(iSocket);
        if (pCa) {

            pCa->iTraiteException() ;

            // Traitement fermeture canal ==> suppression canal
            if (pCa->s == SOCKET_ERROR) {
                bDelCanal(iSocket) ;
                if (pCa->m_bDeleteAuto) {
                    delete pCa ;
                }
            }
        }
        else {
            ERREUR(MASQUE_ERREUR,"EXCEP:Canal non défini pour socket %d",iSocket) ;
            vInitMasques(iSocket,TCP_CLEAR_MASQUE_EXCEPTION) ;
        }
    }
    return ;
}
/*!
********************************************************************************
\brief Récupération de l'adresse d'une machine à partir du nom
\param szMachine Nom de la machine
\return Renvoi l'adresse de la machine / 0 si non trouvé
********************************************************************************
*/
ULONG CSRV_Tcp::ulGetHostByName(LPCSTR szMachine){
    struct hostent *pHostEnt = NULL;

    char acBuf[1024];
    struct hostent ret;
    int iErr;
    if (gethostbyname_r(szMachine, &ret, acBuf, 1024, &pHostEnt, &iErr) != 0) return 0 ;


    if (!pHostEnt)  return 0 ;

    return  *(ULONG*) (pHostEnt->h_addr);
}
