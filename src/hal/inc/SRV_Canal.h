//
// Created by estelle on 04/04/2021.
//

#ifndef PERSO_HNZ_SRV_CANAL_H
#define PERSO_HNZ_SRV_CANAL_H

/*! \file /Inc/Srv/SRV_Canal.h \brief Interfaces des classes CCSLock CSRV_Fifo et CSRV_Canal  */

#include "defs.h"
#include "CriticalSection.h"
#include "StdAfx.h"

// Definition du message de base
//#include "Inc/Msg/msg.h"
//#include "Inc/Utilitaire/EvtFifo.h"
//#include "Inc/Utilitaire/ErreurLog.h"

class CSRV_Tcp ;
class CSRV_Canal ;


#define ETAT_SOCKET_NON_ETABLI	0
#define ETAT_SOCKET_EN_COURS	1
#define ETAT_SOCKET_ETABLI		2
#define ETAT_SOCKET_ERREUR		3

/// Classe utilitaire pour l'entrée en la sortie (automatique) de section critique
class CCSLock {

public:

    CCSLock(CCriticalSection* pLock) {
        m_pLock = pLock ;
        m_pLock->Lock();
    }

    ~CCSLock(){ m_pLock->Unlock(); }

private:
    CCriticalSection* m_pLock;
};




//
// Structure de stockage des messages en fifos
//
struct SNoeud {

    struct  SNoeud *pnoeSvt;	// pointeur noeud suivant

    USHORT 	usLg;				// taille finale

    //-- Ne pas deplacer
    BYTE    ab[2] ;				// Reseve de 2 octets la taille est fixée lors
    // de l'allocation de la structure
}; // noe

/// Classe de gestion des messages FIFO
class CSRV_Fifo  {

// Opérations
public :

    CSRV_Fifo( USHORT usMax = 2000 ) ;
    ~CSRV_Fifo() ;

    struct SNoeud	*pnoeDechainePremier() ;
    void			vDesalloueNoeud(struct SNoeud * pnoe);
    struct SNoeud	*pnoeGetPremier()  { return m_pnoePre ; }
    struct SNoeud	*pnoeAlloue( USHORT usMax )  ;
    BOOL			bAjouteNoeud(struct SNoeud * pnoe )  ;

    int				GetSize   () { return m_usNbNoeud ; } ;
    int				GetMaxSize() { return m_usMaxNoeud ; } ;
    void			SetMaxSize(USHORT usMax ) { m_usMaxNoeud = usMax ;} ;

    // Gestion du multi threading
    //CEvtFifo*		pCreateEvtNonVide(void);
    void			vCreateSectionCritique(void) ;
    void			vSetName(LPCSTR szNom) {
        strncpy(m_szNomFifo, szNom, sizeof(m_szNomFifo));
        m_szNomFifo[sizeof(m_szNomFifo) - 1] = '\0';
    };

protected :

    USHORT	m_usNbNoeud;		// Nb noeuds (celui en cours est compte )
    USHORT	m_usMaxNoeud;		// Nb Mes de saturation
    BOOL	m_bPlein ;			// True si detection plein deja faite (reentrance)
    int		m_iPctUseTrace ;	// Pourcentage utilisé tracé en dizaine de %
    char	m_szNomFifo[30] ;	// Nom de la fifo (traces)

    struct SNoeud *m_pnoePre; 	// premier message, le prochain a sortir
    struct SNoeud *m_pnoeDer; 	// dernier message, le dernier a sortir

    CCriticalSection*	m_pcsAcces ;  // Pointeur sur la SC d'acces a la fifo
    //CEvtFifo		*	m_pevNonVide; // Pointeur sur l'evt a generer sur non vide

    friend	class CSRV_Canal ;
} ;


//
// Codes de retour des fonctions de traitement
//
#define		MSG_BLOQUE		1
#define		MSG_CONTINUE	2

//
//
#define     SATURATION_LECTURE		1
#define     SATURATION_ECRITURE		2

// Lg pour stockage adresse IP ou MAC
#define		MAX_ADRESSE		20

//
//	Classe canal == Gestion d'un socket service ou client
/// Classe de gestion d'un socket service ou client
class CSRV_Canal {

// Operations
public:

    CSRV_Canal();
    virtual ~CSRV_Canal();

    int		iEnvoiMessage(BYTE * pab , USHORT usLg) ;
    int		bInitService(const char *szService, CSRV_Tcp * pSrvTcp, const char *szMachine = NULL, BOOL bLocal = false) ;
    int		bInitClient (const char *szMachine, const char *szService, CSRV_Tcp * pSrvTcp);
    int		bInitClient2(const char *szNomMachineLocal, const char *szNomMachine, const char *szNomService, CSRV_Tcp * pSRV_Tcp, BOOL bConnectNonBloquant=false);
    void	vSetMaxFifos(USHORT usMaxLec , USHORT usMaxEcr ) ;
    //int		iDepileMessageFifo(SRvMESSAGE * pmm) ;

    void	vEnableMultiThread( BOOL bLecture , BOOL bEcriture = FALSE ) ;

    CSRV_Tcp * pGetSrvTcp()  { return m_pInterfaceTcp ;}
    void	vClose() ;

    void	vCSEcriture(BOOL bLock = true)  ;

    BOOL	m_bModeSilencieux;	// TRUE=Pas de message d'echec de connexion

    // Récupération des numéros de ports
    BOOL	bInitSockAddr(LPCSTR szNomMachine, LPCSTR szNomService, struct sockaddr_in & sAddr) ;

    // Lecture adresses
    char*	szGetAdresseIP () { return m_szIP  ; }
    char*	szGetAdresseMAC() { return m_szMAC ; }
    USHORT	sGetAdressePort() { return m_usPORT; }
    char*	szGetAdresseIPLocal () { return m_szIPlocal  ; }
    char*	szGetAdresseMACLocal() { return m_szMAClocal ; }
    USHORT	sGetAdressePortLocal() { return m_usPORTlocal; }

// Overrides
public:
    // Accesseur
    SOCKET sGetSocket() const { return  s; }
    int		iGetEtat() {return m_iEtatConnexionEnCours;}

    // Fonctions appelées à la sortie du select
    int		iTraiteLecture() ;
    virtual	int		iTraiteEcriture() ;
    virtual	int		iTraiteException() ;

    // Fonctions a redéfinir pour implementer un service ou un client
    virtual int		iTraiteMessage(BYTE * /*pab*/, int & /*bStop*/ ) { return MSG_CONTINUE ; }
    virtual void	vTraiteFermeture() { return  ; }
    virtual void	vTraiteException(int iCause, int iTaille) ;
    virtual	void	vSendMessageClient() { return ;}

    virtual CSRV_Canal*	pCreateNouveauCanal() { return NULL ;}


// Implementation
protected:


    // Focntions internes
    SOCKET sAccepteConnexion();

    virtual int	iAssembleTrame(struct SNoeud** ppnoe) ;

    void	vFermeCanal();
    int		bNonBloquant() ;
    int     iTraiteMessagesFifo() ;
    void	vSetAdresses(struct sockaddr_in adresse, LPSTR szMAC, LPSTR szIP, USHORT &usPORT) ;

    int			m_iErreurWSA ;		///< Derniere erreur TCP
    short		m_sPortDefaut ;		///< No de service par defaut
    SOCKET		s  ;				///< Socket correspondant au canal
    BOOL		m_bStop  ;			///< Mettre a vrai pour arreter le traitement
    BOOL		m_bDeleteAuto ;		///< Si Vrai le canal est detruit pas CSRV_Tcp

    CSRV_Fifo   m_ffLecture ;		///< Fifo pour les messages en lecture
    CSRV_Fifo   m_ffEcriture ;		///< Fifo pour les messages en Ecriture

    CSRV_Tcp*   m_pInterfaceTcp ;	///< Pointeur sur l'interface TCP

    int			m_iEtatConnexionEnCours; ///< Etat de la connexion pour un socket. PErtinent pour les sockets non bloquants


    // Noeud sur les messages en cours
    // Ecriture
    struct SNoeud*		m_pnoeEcriture ;
    USHORT				m_usNbDejaEcrits ;
    CCriticalSection*	m_pcsEcriture;     ///< Section critique pour multi thread

    // Lecture
    struct SNoeud*		m_pnoeLecture ;
    unsigned short				m_usNbDejaLus ;
    bool				m_bAttenteTrame;
    bool				m_bDeleteTcp  ;
    bool				m_bConnexion ;		///< Si socket de connexion
    bool				m_bEnteteFormatSRV;	///< On gere des SRvMESSAGEs

    //v4.0 Ajout entete
    //SRvENTETE			m_entRecup ;
    //RvENTETE			m_entMemoire ;

    // Adresses IP & MAC
    char				m_szIP [MAX_ADRESSE] ;	///< @ IP réelle
    char				m_szMAC[MAX_ADRESSE] ;	///< @ MAC réelle
    unsigned short 				m_usPORT;				///< No de service réel
    char				m_szIPlocal [MAX_ADRESSE] ;	///< @ IP réelle
    char				m_szMAClocal[MAX_ADRESSE] ;	///< @ MAC réelle
    USHORT				m_usPORTlocal;				///< No de service réel

    friend class CSRV_Tcp ;
};

#undef AFX_DATA
#define AFX_DATA

#endif //PERSO_HNZ_SRV_CANAL_H
