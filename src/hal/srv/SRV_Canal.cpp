vCSEcriture(false) ;//
// Created by estelle on 04/04/2021.
//

#include "../inc/SRV_Canal.h"

/*! \file /Com/Srv/SRV_Canal.cpp \brief Classe CSRV_Canal et CSRV_Fifo */
/*!
*******************************************************************************************
\class CSRV_Canal
Cette classe gère un canal socket entre un client et un serveur
*******************************************************************************************
*/

#include "../inc/StdAfx.h"

#include <stdlib.h>

#include "../inc/SRV_Tcp.h"

//#include "Inc/Utilitaire/RegistryXML.h"
//#include "Inc/FichierNoyau.h"

#define GetErreur() errno
#include <arpa/inet.h>
#include <net/if_arp.h>
#include <net/if.h>


/*!
********************************************************************************
\brief Constructeur de l'objet Canal ==> initialisations
********************************************************************************
*/
CSRV_Canal::CSRV_Canal(){

    // Socket non initialisée
    s = INVALID_SOCKET ;
    m_iEtatConnexionEnCours = ETAT_SOCKET_NON_ETABLI ;

    // Ce n'est pas un socket pendant
    m_bConnexion	= false ;

    // Pas de port tcp affecté
    m_sPortDefaut = 0 ;

    // Init messages en ecriture
    m_pnoeEcriture = NULL ;
    m_usNbDejaEcrits = 0 ;

    // Init messages en lecture
    m_pnoeLecture = NULL ;
    m_usNbDejaLus = 0 ;
    m_bAttenteTrame = false ;

    memset(&m_entRecup,0,sizeof(m_entRecup)) ;

    m_pInterfaceTcp = NULL ;
    m_bDeleteTcp = FALSE ;
    m_bDeleteAuto = false ;
    m_bStop = false ;
    m_bModeSilencieux = false;

    m_pcsEcriture = NULL ;

    m_iErreurWSA = 0 ;

    m_ffLecture.vCreateSectionCritique();
    m_ffEcriture.vCreateSectionCritique();
    m_pcsEcriture = new CCriticalSection ;

    // Maj nom des fifos
    m_ffEcriture.vSetName("Ecriture") ;
    m_ffLecture .vSetName("Lecture") ;

    // Init adresses
    memset(m_szIP ,0,sizeof(m_szIP )) ;
    memset(m_szMAC,0,sizeof(m_szMAC)) ;
    m_usPORT = 0;
    memset(m_szIPlocal ,0,sizeof(m_szIPlocal )) ;
    memset(m_szMAClocal,0,sizeof(m_szMAClocal)) ;
    m_usPORTlocal = 0;

    // Par défaut on ne gère pas des SRvMESSAGE
    m_bEnteteFormatSRV	= false ;

}

/*!
********************************************************************************
\brief Destructeur de l'objet canal
********************************************************************************
*/
CSRV_Canal::~CSRV_Canal(){

    // Nettoyage
    vFermeCanal() ;

    // Destruction de la section critique
    delete m_pcsEcriture ;
    m_pcsEcriture = NULL;	//On met NULL dans le pointeur pour savoir qu'il a ete delete (utile pour debug)

    if (m_bDeleteTcp ) {
        delete m_pInterfaceTcp ;
        m_pInterfaceTcp  = NULL ;
    }

    // Rq: Les noeuds en fifo sont détruits par les destructeurs des fifo(s)
}

/*!
********************************************************************************
\brief Passe le socket en non bloquant en lecture et écriture
\return Renvoi vrai en cas de succès, renvoi faux sinon
********************************************************************************
*/
int CSRV_Canal::bNonBloquant() {


    int iVal = 1 ;
    if (ioctl(s,FIONBIO,&iVal) == SOCKET_ERROR) {

        m_iErreurWSA = GetErreur() ;

        // Sur Pb fermeture du socket
        vFermeCanal();

        if (!m_bModeSilencieux) ERREUR(MASQUE_ERREUR_CRITIQUE, "Erreur Non Bloquant %d sur socket %d",m_iErreurWSA,s);
        else ERREUR(MASQUE_DEBUG, "Erreur Non Bloquant %d sur socket %d",m_iErreurWSA,s);

        return false;
    }
    return true ;
}

/*!
********************************************************************************
\brief Close du socket
********************************************************************************
*/
void CSRV_Canal::vFermeCanal(){

    // Si pas fermee ==> fermeture socket
    if (s != INVALID_SOCKET) {

        // Raz des masques pour le select
        if (m_pInterfaceTcp) {
            m_pInterfaceTcp->bSupprimeCanalService(this) ;
        }

        // Fermeture socket
        vClose();

        // Traitement clients
        vTraiteFermeture() ;

        // Noeuds en cours
        if (m_pnoeEcriture != NULL) m_ffEcriture.vDesalloueNoeud(m_pnoeEcriture) ;
        if (m_pnoeLecture  != NULL)  m_ffLecture.vDesalloueNoeud(m_pnoeLecture) ;

        m_pnoeLecture = m_pnoeEcriture = NULL ;

        struct SNoeud * pnoe ;

        // Vidange des fifos
        while ((pnoe = m_ffEcriture.pnoeDechainePremier()) != NULL)
            m_ffEcriture.vDesalloueNoeud(pnoe) ;

        while ((pnoe = m_ffLecture.pnoeDechainePremier())  != NULL)
            m_ffLecture.vDesalloueNoeud(pnoe) ;

        m_usNbDejaLus = 0;
    }
}

/*!
********************************************************************************
\brief Depile et traite les messages en attente dans la fifo
\return Renvoi le type de message
********************************************************************************
*/
int CSRV_Canal::iTraiteMessagesFifo() {

    struct SNoeud* pnoe ;
    int	iTrt = MSG_CONTINUE;

    m_bStop = false ;

    // parcours de la fifo
    while (!m_bStop && (pnoe = m_ffLecture.pnoeGetPremier()) != NULL) {

        // Traiter le message
        iTrt = iTraiteMessage(pnoe->ab, m_bStop) ;

        if (iTrt != MSG_BLOQUE) {
            pnoe = m_ffLecture.pnoeDechainePremier() ;
            m_ffLecture.vDesalloueNoeud(pnoe) ;
        }
    }

    return iTrt ;
}

/*!
********************************************************************************
\brief Dépile et traite un message en attente dans la fifo
\param pmm Message de réception
\return Renvoi 0
********************************************************************************
*/
int CSRV_Canal::iDepileMessageFifo(SRvMESSAGE * pmm) {

    struct SNoeud * pnoe ;

    // Depile la fifo
    pnoe = m_ffLecture.pnoeDechainePremier();

    if (pnoe == NULL ) return -1 ;

    // Recopie le message
    memcpy(pmm,pnoe->ab,pnoe->usLg) ;
    m_ffLecture.vDesalloueNoeud(pnoe) ;

    return 0 ;
}

/*!
********************************************************************************
\brief Fonction appelée lors de la reception de données sur le socket.
Elle demande l'assemblage des messages puis appelle la fonction virtuelle de traitement des messages.
Les Messages bloqués par le client sont empilés dans la fifo de lecture.
\return 0 en cas de succès, -1 en cas d'erreur
********************************************************************************
*/
int CSRV_Canal::iTraiteLecture() {

    int				iMsgLu ;
    int				iTrt ;
    struct SNoeud*	pnoe ;

    // Test lecture sur socket pendant ==> accept + traitement message de connexion
    if (m_bConnexion)  {

        SOCKET sClient ;

        sClient = sAccepteConnexion() ;

        // Test erreurs tcp
        if ( sClient == SOCKET_ERROR) 	return -1 ;

        // Socket Ok on crée le canal client
        CSRV_Canal* pCanalClient = pCreateNouveauCanal() ;

        // Refus création du contexte, on jette le client potentiel
        if (pCanalClient == NULL) {
            closesocket(sClient) ;
            return 0;
        }

        // Init de la Connexion client
        pCanalClient->s = sClient ;
        pCanalClient->m_bDeleteAuto = true ;
        pCanalClient->m_pInterfaceTcp = m_pInterfaceTcp ;

        strcpy(pCanalClient->m_szIP,m_szIP) ;
        strcpy(pCanalClient->m_szMAC,m_szMAC) ;
        pCanalClient->m_usPORT = m_usPORT;
        strcpy(pCanalClient->m_szIPlocal,m_szIPlocal) ;
        strcpy(pCanalClient->m_szMAClocal,m_szMAClocal) ;
        pCanalClient->m_usPORTlocal = m_usPORTlocal;

        // Ajout de la connexion dans l'interface TCP
        if (!m_pInterfaceTcp->bAjouteCanalService(pCanalClient)) {
            if (pCanalClient->m_bDeleteAuto) delete pCanalClient ;
            return -1 ;
        }

        // Passage canal en mode non bloquant
        pCanalClient->bNonBloquant() ;

        // Gestion du message de connexion
        ERREUR(MASQUE_DEBUG,"Connexion client") ;
        pCanalClient->iTraiteLecture() ;

        // Test canal client ferme par le client
        if (pCanalClient->s == INVALID_SOCKET) {
            if (pCanalClient->m_bDeleteAuto) delete pCanalClient ;
            return -1 ;
        }

        return 0  ;
    }

        // Message normal recu sur un socket
    else {
        m_bStop = false ;

        // Recup des messages su rle socket
        while ( (iMsgLu = iAssembleTrame(&pnoe)) > 0 ) {

            // Traitement du dernier message lu
            iTrt = iTraiteMessage(pnoe->ab,m_bStop) ;

            if (iTrt == MSG_BLOQUE) {
                // Si demande de bloquage on le garde pour plus tard
                m_bAttenteTrame = true ;

                if (!m_ffLecture.bAjouteNoeud(pnoe)) {
                    // Saturation de la fifo
                    m_ffLecture.vDesalloueNoeud(pnoe) ;

                    // Traitement Exception
                    vTraiteException(SATURATION_LECTURE, m_ffLecture.GetMaxSize()) ;
                    return 0 ;
                }
                pnoe = NULL ;

                // Appel envoi message pour les clients windows !
                vSendMessageClient()  ;

            }
            else {
                // Message consommé ==> destruction
                if (pnoe != NULL) m_ffLecture.vDesalloueNoeud(pnoe) ;
                pnoe = NULL ;
                m_bAttenteTrame = false ;

                // Passage canal suivant
                return 0 ;
            }

            if (m_bStop) return iTrt ;
        }

        // Perte de connection
        if (iMsgLu < 0 ) {
            vFermeCanal() ;
            return -1 ;
        }

        // Si on attend pas un message specifique et qu'il en reste en fifo ==> depile
        // if (!bAttenteTrame) iTraiteMessagesFifo() ;

        return 0 ;
    }
}

/*!
********************************************************************************
\brief Envoi d'une trame dans le socket
	Cette fonction est appelée :
		- soit à la suite du select
		- soit par la fonction d'envoi d'un message
\return 0 en cas de succès, -1 en cas d'erreur
********************************************************************************
*/
int CSRV_Canal::iTraiteEcriture() {

    int		iNbEcrits ;
    int		iRestant ;

    // Connexion en non bloquant
    if (m_iEtatConnexionEnCours == ETAT_SOCKET_EN_COURS) {


        if (m_pInterfaceTcp) m_pInterfaceTcp->vInitMasques(this, TCP_MASQUE_LECTURE	| TCP_CLEAR_MASQUE_ECRITURE	| TCP_CLEAR_MASQUE_EXCEPTION);

        m_iEtatConnexionEnCours = ETAT_SOCKET_ETABLI;

        // Récupération config locale
        struct sockaddr_in  myaddrLocale;  // remote socket address
        int iI = sizeof(myaddrLocale);

        getsockname(s, (struct sockaddr*)&myaddrLocale, (socklen_t*)&iI);

        vSetAdresses(myaddrLocale, m_szMAClocal, m_szIPlocal, m_usPORTlocal) ;

        ERREUR(MASQUE_TRACE,"Connect réussi MAC=[%s] IP=[%s] Port=[%hd] (local MAC=[%s] IP=[%s] Port=[%hd] Socket=[%d])",
               m_szMAC, m_szIP, m_usPORT, m_szMAClocal, m_szIPlocal, m_usPORTlocal, s);

        return 0;
    }


    // Envoi des messages
    do {
        // Passage non bloquant
        if (!bNonBloquant()) return -1 ;

        // Reset nb deja ecrits
        iNbEcrits = 0 ;

        // Si pas de message en cours en depiler un
        if (m_pnoeEcriture == NULL ) {
            m_pnoeEcriture = m_ffEcriture.pnoeDechainePremier();
            m_usNbDejaEcrits = 0 ;
        }

        // Test si message à envoyer
        if (m_pnoeEcriture != NULL) {

            // Envoi du message
            iRestant = m_pnoeEcriture->usLg - m_usNbDejaEcrits ;

            // Ecriture sur le socket
            if (iRestant != 0)
                iNbEcrits = send(s,(char*)&m_pnoeEcriture->ab[m_usNbDejaEcrits],iRestant,0) ;

            if (iNbEcrits >0 ) {
                m_usNbDejaEcrits += iNbEcrits ;

                // Teste si tout est parti
                if (m_usNbDejaEcrits == m_pnoeEcriture->usLg) {
                    m_ffEcriture.vDesalloueNoeud(m_pnoeEcriture) ;
                    m_pnoeEcriture = NULL ;
                }
            }
        }

    } while(iNbEcrits >0) ;

    // Teste si erreur sur le socket
    if (iNbEcrits == SOCKET_ERROR) {

        m_iErreurWSA = GetErreur() ;

        if ( m_iErreurWSA !=  WSAEWOULDBLOCK ) {
            vFermeCanal() ;
            ERREUR(MASQUE_ERREUR_CRITIQUE,"Erreur socket <0x%X> sur ecriture",m_iErreurWSA) ;
            return -1 ;
        }
    }

    // Si il en reste alors mettre le socket dans le masque d'ecriture
    if (m_pInterfaceTcp != NULL) {
        if (m_pnoeEcriture == NULL) {
            m_pInterfaceTcp->vInitMasques(this, TCP_CLEAR_MASQUE_ECRITURE);
        }
        else
            m_pInterfaceTcp->vInitMasques(this, TCP_MASQUE_ECRITURE);
    }

    return 0 ;
}

/*!
********************************************************************************
\brief Envoi de message
\param pab Le pointeur sur le message à envoyer
\param usLg La taille du message
\return 0 en cas de succès, -2 en cas de saturation d'écriture, -1 sinon
********************************************************************************
*/
int CSRV_Canal::iEnvoiMessage(BYTE * pab , USHORT usLg) {

    struct SNoeud * pnoe ;
    USHORT usL ;

    if (pab == NULL) {
        ERREUR(MASQUE_ERREUR_CRITIQUE,"Refus ecriture pointeur nul") ;
        return -1 ;
    }

    // si usLg != 0 le prendre comme taille,
    // sinon prendre la taille dans le message
    /*if (m_bEnteteFormatSRV) {
        usL = usLg ? usLg : ((SRvENTETE*)pab)->usLg;

        // Swap de l'entete avant emission
        ((SRvENTETE*)pab)->vEncode();
    }*/
    else
        usL = usLg ;

    // Creation du message
    if (usL == 0 ) {
        ERREUR(MASQUE_ERREUR,"Refus ecriture d'un message de taille nulle") ;
        return -1 ;
    }

    // Entree en section critique (pour multithread )
    vCSEcriture(true) ;

    // Allocation du noeud
    pnoe = m_ffEcriture.pnoeAlloue(usL) ;
    if (pnoe == NULL ) {
        ERREUR(MASQUE_ERREUR_CRITIQUE,"pnoeAjoute::Erreur allocation memoire") ;
        vCSEcriture(false) ;
        return -1 ;
    }
    // Recopie des infos
    memcpy( pnoe->ab , pab ,  usL );

    // Swap de l'entete apres memo pour ne pas
    // modifier les messages utilisateur en
    // cas d'emissions multiples
    if (m_bEnteteFormatSRV) ((SRvENTETE*)pab)->vDecode();

    // Envoi du message
    if ( m_pnoeEcriture != NULL) {
        // Il y a deja un message en cours d'ecriture ==> mise en fifo
        if (!m_ffEcriture.bAjouteNoeud(pnoe)) {
            m_ffEcriture.vDesalloueNoeud(pnoe) ;

            // Traitement Exception
            vTraiteException(SATURATION_ECRITURE, m_ffEcriture.GetMaxSize()) ;

            // Sortie de section critique (pour multithread )
            vCSEcriture(false) ;
            return -2 ;
        }
    }
    else {
        m_pnoeEcriture = pnoe ;
        m_usNbDejaEcrits = 0 ;
    }

    // Essai d'envoi du message
    int iRet = iTraiteEcriture() ;

    // Sortie de section critique (pour multithread )
    vCSEcriture(false) ;

    return iRet ;
}

/*!
********************************************************************************
\brief Traitement des exceptions sur un socket durant le select
\return 0
********************************************************************************
*/
int CSRV_Canal::iTraiteException() {

    // Connexion en non bloquant
    if (m_iEtatConnexionEnCours == ETAT_SOCKET_EN_COURS) {

        if (!m_bModeSilencieux) ERREUR(MASQUE_ERREUR,"Echec Connect vers IP=[%s] Port=[%hd]", m_szIP, m_usPORT);
        else ERREUR(MASQUE_DEBUG,"Echec Connect vers IP=[%s] Port=[%hd]", m_szIP, m_usPORT);
        m_pInterfaceTcp->bSupprimeCanalService(this);

        // Fermeture socket
        closesocket(s) ;
        s = INVALID_SOCKET ;
        m_iEtatConnexionEnCours = ETAT_SOCKET_ERREUR ;

        return 0;
    }

    ERREUR(MASQUE_ERREUR,"TraiteException : (Err=%d)",m_iErreurWSA) ;

    return 0 ;
}

/*!
********************************************************************************
\brief Recupère des octets sur le socket et essai de fabriquer un message.
	Les messages traités commencent tous par un octet indiquant la longueur du message.
\param ppnoe L'adresse du pointeur de noeud
\return 1 en cas de succès, 0 ou une valeur négative en cas d'erreur
********************************************************************************
*/
int CSRV_Canal::iAssembleTrame(struct SNoeud** ppnoe) {

    int		iNbLus 	= 0;
    int		iNbALire= 0;
    int		iRet	= 0 ;
    USHORT	usLgMsg  ;
    char*	pab ;

    // Init errno
    errno = 0 ;
    m_iErreurWSA = 0 ;

    // Retour = NULL
    *ppnoe = NULL ;

    if ( s == INVALID_SOCKET) return 0;

    // Entree en section critique (pour multithread )
    vCSEcriture(true) ;

    do {
        // Si message en cours on continue sur celui ci
        if (m_pnoeLecture != NULL) {

            // Calcul du nombre restant a lire
            iNbALire = m_pnoeLecture->usLg - m_usNbDejaLus ;

            // Lecture socket
            if (iNbALire > 0) {

                // Lecture Socket
                iNbLus = recv(s,(char*)&m_pnoeLecture->ab[m_usNbDejaLus],iNbALire,0) ;

                // Test retour
                if (iNbLus <= 0 ) continue ;

                m_usNbDejaLus +=  iNbLus ;
            }

            // Test si fin du message
            if (m_usNbDejaLus == m_pnoeLecture->usLg ) 	{
                iRet = 1 ;
                m_usNbDejaLus = 0 ;
                goto finAssemble ;
            }

        }
        else {   // Pas de message en cours on initialise le message à lire

            // Il faut lire l'entete ou ses restes
            // Calcul du nombre restant a lire
            iNbALire = sizeof(SRvENTETE)  - m_usNbDejaLus ;

            pab = (char*)&m_entRecup ;

            // Mémorisation pour debug
            m_entMemoire = m_entRecup;

            iNbLus = recv(s,&pab[m_usNbDejaLus],iNbALire,0) ;

            // Recup de la taille
            if (iNbLus == iNbALire) {
                // Swap en tete a la reception
                m_entRecup.vDecode();

                // calcul de la taille du message
                usLgMsg = m_entRecup.usLg ;

                if ((usLgMsg < sizeof(SRvENTETE) ) || (usLgMsg > SRvMESSAGE_MAX)) {

                    ERREUR(MASQUE_ERREUR_CRITIQUE,"Message recu de taille incorrecte Lg=%hd - Cde=%hd - Msg=%hd - Client=%hd - Lu=%d",
                           usLgMsg, m_entRecup.sCde, m_entRecup.usMessage, m_entRecup.usClient, iNbLus) ;
                    ERREUR(MASQUE_ERREUR_CRITIQUE,"Message precedent Lg=%hd - Cde=%hd - Msg=%hd - Client=%hd",
                           m_entMemoire.usLg, m_entMemoire.sCde, m_entMemoire.usMessage, m_entMemoire.usClient) ;

                    vFermeCanal() ;
                    iRet = -1 ;
                    goto finAssemble ;
                }

                // Allocation de l'espace pour le message
                m_pnoeLecture = m_ffLecture.pnoeAlloue(usLgMsg) ;
                if (m_pnoeLecture == NULL) {
                    ERREUR(MASQUE_ERREUR_CRITIQUE,"pnoeAjoute::Erreur allocation memoire") ;
                    exit(1) ;
                }

                // Recopie de l'entete
                memcpy(&m_pnoeLecture->ab,&m_entRecup,sizeof(m_entRecup)) ;
                m_usNbDejaLus = sizeof(SRvENTETE);
            }
            else {
                m_iErreurWSA = GetErreur() ;
                if ( m_iErreurWSA != WSAEWOULDBLOCK )	iRet = -1 ;

                // Ajout test cas particulier Erreur = 0 et octets lus
                if (iNbLus != 0 && m_iErreurWSA == 0) iRet = 0 ;

                if (iNbLus > 0) m_usNbDejaLus += iNbLus ; else m_usNbDejaLus = 0 ;
                goto finAssemble ;
            }
        }

    } while ((iNbLus > 0)) ;

    // Si erreur TCP ==> retour -1
    if (iNbLus == SOCKET_ERROR) {
        m_iErreurWSA = GetErreur() ;
        if ( m_iErreurWSA !=  WSAEWOULDBLOCK ) iRet = -1 ;
    }
    else
    if (iNbLus == 0 ) {
        // Socket fermé !!!
        vFermeCanal() ;
        iRet = -1 ;
    }


    finAssemble:

    // Positionne la variable de retour
    if ( iRet == 1) {
        *ppnoe = m_pnoeLecture ;
        m_pnoeLecture = NULL ;
    }

    // Sortie de section critique (pour multithread )
    vCSEcriture(false) ;

    return iRet ;
}

/*!
*****************************************************************************
\class CSRV_Fifo
Classe de gestion des messages fifos
*****************************************************************************
*/

/*!
********************************************************************************
\brief Constructeur de l'objet fifo.
\param usMax Le numbre maximum de noeuds
********************************************************************************
*/
CSRV_Fifo::CSRV_Fifo( USHORT usMax ) {

    // Init des variables internes
    m_pnoeDer		= NULL ;
    m_pnoePre		= NULL ;
    m_usMaxNoeud	= usMax ;
    m_usNbNoeud		= 0 ;
    m_bPlein		= false ;

    m_pcsAcces		= NULL ;
    m_pevNonVide	= NULL ;

    // Maj pour traces
    vSetName("SANS NOM") ;
    m_iPctUseTrace  = 0 ;
}

/*!
********************************************************************************
\brief Destructeur de l'obet fifo supprime les messages qui restent.
********************************************************************************
*/
CSRV_Fifo::~CSRV_Fifo () {
    struct SNoeud * pnoe ;

    // Destruction de la fifo
    while ( (pnoe = pnoeDechainePremier()) != NULL) vDesalloueNoeud(pnoe) ;

    if (m_pcsAcces != NULL) delete m_pcsAcces;
    if (m_pevNonVide != NULL) delete m_pevNonVide;
}

/*!
********************************************************************************
\brief Supprime le premier element de la fifo
\return Le noeud dépilé
********************************************************************************
*/
struct SNoeud *CSRV_Fifo::pnoeDechainePremier() {

    struct SNoeud * pnoe  = NULL ;
    int				iPct ;

    // Protection pour multithread
    if (m_pcsAcces != NULL) m_pcsAcces->Lock() ;

    if (m_usNbNoeud != 0 ) {
        pnoe = m_pnoePre ;
        m_pnoePre = pnoe->pnoeSvt ;
        m_usNbNoeud -- ;

        // Mise a jour pourcentage utilisé en dizaine de %
        iPct = (10 * m_usNbNoeud) / m_usMaxNoeud ;
        if (iPct != m_iPctUseTrace) {
            m_iPctUseTrace = iPct ;
            ERREUR(MASQUE_TRACE_SRV,"%d %% de la fifo %s utilisee %hd / %hd", iPct*10,m_szNomFifo,m_usNbNoeud,m_usMaxNoeud) ;
        }

        // Reset de l'evt non vide si necessaire
        if (m_pevNonVide != NULL){
            if (m_usNbNoeud == 0)
                m_pevNonVide->ResetEvent();
            else
                m_pevNonVide->SetEvent();
        }
    }

    // Fin protection pour multithread
    if (m_pcsAcces != NULL) m_pcsAcces->Unlock() ;

    return pnoe ;
}

/*!
********************************************************************************
\brief Alloue un noeud de taille donnée
\param usMax La taille du noeud à allouer
\return Le noeud alloué
********************************************************************************
*/
struct SNoeud *CSRV_Fifo::pnoeAlloue( USHORT usMax) {

    struct SNoeud * pnoe ;

    // Protection pour multithread
    if (m_pcsAcces != NULL) m_pcsAcces->Lock() ;

    pnoe =  (SNoeud*) malloc(sizeof(struct SNoeud) + usMax);
    if (pnoe) {
        // Allocation réussie
        pnoe->pnoeSvt = NULL;   // pas de suivant a l'allocation
        pnoe->usLg = usMax;     // initialisation taille utile du message
    }

    // Fin protection pour multithread
    if (m_pcsAcces != NULL) m_pcsAcces->Unlock() ;

    return pnoe ;
}

/*!
********************************************************************************
\brief Ajoute le noeud en tete si il reste de la place
\param pnoe Le noeud à ajouter
\return Vrai en cas de succès
********************************************************************************
*/
BOOL CSRV_Fifo::bAjouteNoeud(struct SNoeud * pnoe ) {

    BOOL bRet = true ;
    int iPct ;

    // Protection pour multithread
    if (m_pcsAcces != NULL) m_pcsAcces->Lock() ;

    // Test ok
    if ((pnoe == NULL) || ( m_usNbNoeud >= m_usMaxNoeud)) {
        if (!m_bPlein) {
            m_bPlein = true ;
        }
        bRet = false ;
    }

        // Gestion flag fifo plein pour pb reetrance sur message d'erreur
    else {
        m_bPlein = false ;

        if (m_usNbNoeud++ == 0)	// premier SNoeud
            m_pnoePre = pnoe ;
        else
            m_pnoeDer -> pnoeSvt  = pnoe ;

        // Mise a jour pourcentage utilisé en dizaine de %
        iPct = (10 * m_usNbNoeud) / m_usMaxNoeud ;
        if (iPct != m_iPctUseTrace) {
            m_iPctUseTrace = iPct ;
            ERREUR(MASQUE_TRACE_SRV,"%d %% de la fifo %s utilisee %hd / %hd", iPct*10,m_szNomFifo,m_usNbNoeud,m_usMaxNoeud) ;
        }

        // C'est le dernier alloué
        m_pnoeDer = pnoe ;

        if (m_pevNonVide != NULL) m_pevNonVide->SetEvent();
    }

    // Fin protection pour multithread
    if (m_pcsAcces != NULL) m_pcsAcces->Unlock() ;

    return bRet ;
}

/*!
********************************************************************************
\brief Libère la mémoire allouée pour le noeud
\param pnoe Le noeud à supprimer
********************************************************************************
*/
void CSRV_Fifo::vDesalloueNoeud(struct SNoeud * pnoe){
    // On s'assure que le noeud existe avant de le supprimer
    if (pnoe != NULL) {
        free(pnoe) ;
    }
}

/*!
********************************************************************************
\brief Crée un événement pour fifo non vide
\return L'événement existant ou créé
********************************************************************************
*/
CEvtFifo * CSRV_Fifo::pCreateEvtNonVide (){

    // Creation Event
    if ( m_pevNonVide == NULL ) {
        m_pevNonVide = new CEvtFifo ;
    }

    return m_pevNonVide ;
}

/*!
********************************************************************************
\brief Crée une section critique d'accès à la fifo
********************************************************************************
*/
void CSRV_Fifo::vCreateSectionCritique (){
    // Creation Section critique
    if (m_pcsAcces == NULL ) {
        m_pcsAcces = new CCriticalSection ;
    }
}

/*!
********************************************************************************
\brief Création du socket en écoute sur une adresse et une no de port TCP.
\param szNomService Le nom du service
\param pSRV_Tcp L'interface de service
\param szNomMachine Le nom de la machine
\return Vrai en cas de succès
********************************************************************************
*/
int CSRV_Canal::bInitService(const char *szNomService , CSRV_Tcp * pSRV_Tcp, const char* szNomMachine, BOOL bLocal) {

    struct servent *sp;			// ptr to service information
    struct sockaddr_in myaddr;	// local socket address
    ULONG	ulAddr;				// Adresse IP du serveur distant

    // Le nom de service est une chaine nulle, pas un pointeur null
    char acNULL = '\0';
    if (szNomService == NULL) szNomService = &acNULL;

    // Famille AF_INET
    myaddr.sin_family = AF_INET;

    // Determination du nom de la machine
    // Dans le cas ou le PC possède plusieurs adresses IP, on peut choisir
    // l'adresse sur laquelle on attend
    // Si on ne fournit pas de nom de machine alors on attend sur toutes les
    // adresses connues de la machine
    if ((szNomMachine == NULL) || (strlen(szNomMachine)==0)){
        if(!bLocal) {
            myaddr.sin_addr.s_addr = INADDR_ANY;
        }else {
            myaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
        }
    }
    else {

        if (pSRV_Tcp)
            ulAddr = pSRV_Tcp->ulGetHostByName(szNomMachine);
        else
            ulAddr  = 0 ;

        if ( ulAddr == 0) {
            m_iErreurWSA = GetErreur() ;
            ERREUR(MASQUE_ERREUR,"Erreur gethostbyname %d de [%s]",m_iErreurWSA, szNomMachine) ;
            return false ;
        }

        // Recup de l'adresse
        if(!bLocal) {
            myaddr.sin_addr.s_addr = ulAddr;
        }else {
            myaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
        }
    }

    // Recup des infos sur le service utilisé dans ../etc/services
    myaddr.sin_port = htons(m_sPortDefaut) ;

    // Lecture etc/services
    sp = getservbyname(szNomService, "tcp");
    if (sp == NULL) {
        // Si pas de port par défaut configuré
        if (m_sPortDefaut == 0 ) {
            m_iErreurWSA = GetErreur() ;
            ERREUR(MASQUE_ERREUR, "%s/tcp service non trouve %d",szNomService,m_iErreurWSA) ;
            return false ;
        }
    }
    else myaddr.sin_port = sp->s_port;

    /* Create the stream socket */
    s = socket(myaddr.sin_family, SOCK_STREAM, 0);
    if (s == SOCKET_ERROR) {
        ERREUR(MASQUE_ERREUR, " %s: unable to create datagram socket",szNomService);
        return false ;
    }

    // Mise à jour des options
    BOOL  bOpt = 1 ;
    if (setsockopt( s, SOL_SOCKET, SO_REUSEADDR, (const char*)&bOpt,sizeof(bOpt))!=0){
        ERREUR(MASQUE_ERREUR, " %s: operation setsockopt refusee",szNomService);
        return false ;
    }

    // Parametrage de la taille des buffers
    int iTaille = SRvMESSAGE_MAX ;
    if (setsockopt( s, SOL_SOCKET, SO_SNDBUF, (const char*)&iTaille,sizeof(iTaille))!=0){
        ERREUR(MASQUE_ERREUR, " %s: operation setsockopt SO_SNDBUF refusee",szNomService);
    }

    if (setsockopt( s, SOL_SOCKET, SO_RCVBUF, (const char*)&iTaille,sizeof(iTaille))!=0){
        ERREUR(MASQUE_ERREUR, " %s: operation setsockopt refusee",szNomService);
    }

#ifdef _HPUX_SOURCE
    if ((bind( s ,&myaddr, sizeof(myaddr))) < 0 ) {
#else
    if ((bind( s ,(struct sockaddr*) &myaddr, sizeof(myaddr))) < 0 ) {
#endif
        m_iErreurWSA = GetErreur() ;
        ERREUR(MASQUE_ERREUR, " %s: operation bind refusee. Erreur %d", szNomService, m_iErreurWSA);
        return false ;
    }

    // Listen call doesn't apply to datagram sockets
    if (listen(s, 5) == -1 ) {
        ERREUR(MASQUE_ERREUR, "%s: unable to listen",szNomService);
        return false ;
    }

    // Passage le socket en non bloquant
    if (!bNonBloquant()) return false ;

    // Pointeur sur l'interface du service
    m_pInterfaceTcp = pSRV_Tcp ;

    return true ;
}

/*!
********************************************************************************
\brief Accepte la connexion d'un socket sur le socket service
\return Le socket client
********************************************************************************
*/
SOCKET CSRV_Canal::sAccepteConnexion(){
    int iTaille;

    struct sockaddr_in  peer;  // remote socket address
    SOCKET sClient ;

    iTaille = sizeof(struct sockaddr);
    memset(&peer,0,iTaille);

    sClient = accept(s,(struct sockaddr*) &peer,(socklen_t*)&iTaille );


    // Test erreurs tcp
    if ( sClient == SOCKET_ERROR) {
        m_iErreurWSA = GetErreur() ;
        ERREUR(MASQUE_ERREUR,"Erreur <%d> sur accept",m_iErreurWSA);
        return SOCKET_ERROR;
    }

    // MAJ Adresses IP & MAC
    vSetAdresses(peer, m_szMAC, m_szIP, m_usPORT) ;

    // Récupération config locale
    struct sockaddr_in  myaddrLocale;  // remote socket address
    int iI = sizeof(myaddrLocale);

    getsockname(sClient, (struct sockaddr*)&myaddrLocale, (socklen_t*)&iI);

    vSetAdresses(myaddrLocale, m_szMAClocal, m_szIPlocal, m_usPORTlocal) ;

    ERREUR(MASQUE_TRACE,"Accept MAC=[%s] IP=[%s] Port=[%hd] (local MAC=[%s] IP=[%s] Port=[%hd])",
           m_szMAC, m_szIP, m_usPORT, m_szMAClocal, m_szIPlocal, m_usPORTlocal);

    return sClient ;
}

/*!
********************************************************************************
\brief Init du service Tcp. Connexion sur un socket distant en ecoute
\param szNomMachine Le nom de la machine
\param szNomService Le nom du service
\param pSRV_Tcp L'interface TCP
\return Vrai en cas de succès
********************************************************************************
*/
int  CSRV_Canal::bInitClient(const char *szNomMachine, const char *szNomService, CSRV_Tcp * pSRV_Tcp) {

    return bInitClient2("", szNomMachine, szNomService, pSRV_Tcp, false);
}

/*!
********************************************************************************
\brief Init du service Tcp. Connexion sur un socket distant en ecoute
\param szNomMachineLocale Le nom de la machine local permettant de choisir l'@ IP de sortie
\param szNomMachine Le nom de la machine
\param szNomService Le nom du service
\param pSRV_Tcp L'interface TCP
\param bConnectNonBloquant Indique si l'on utilise le mode d'ouverture non bloquant
\return Vrai en cas de succès
********************************************************************************
*/
int  CSRV_Canal::bInitClient2(const char *szNomMachineLocale, const char *szNomMachine, const char *szNomService, CSRV_Tcp * pSRV_Tcp, BOOL bConnectNonBloquant) {

    struct servent *sp;			// pointer to service information
    struct sockaddr_in myaddr;	// for peer socket address
    struct sockaddr_in myaddrLocale;	// Pour @ IP locale

    // Le nom de service est une chaine nulle, pas un pointeur null
    char acNULL = '\0';
    if (szNomService == NULL) szNomService = &acNULL;

    // Pointeur sur l'interface du service
    if ((pSRV_Tcp == NULL) && (m_pInterfaceTcp == NULL)){
        m_bDeleteTcp = TRUE ;
        pSRV_Tcp     = new CSRV_Tcp ;
    }

    if (pSRV_Tcp) m_pInterfaceTcp = pSRV_Tcp ;

    // Traitement du nom de la machine

    if ((szNomMachine == NULL) || (strlen(szNomMachine) ==0)) {     // On utilise la loopback pour le local (pb 2000/XP)
        myaddr.sin_addr.s_addr  =  htonl(INADDR_LOOPBACK) ;
    }
    else {

        // Test si @ IP
        myaddr.sin_addr.s_addr  = inet_addr(szNomMachine) ;

        // Test adresse ou hostname
        if (myaddr.sin_addr.s_addr == htonl(INADDR_NONE)) {

            // Resolution du nom
            ULONG ulAddr = m_pInterfaceTcp->ulGetHostByName(szNomMachine);
            if ( ulAddr == 0) {
                m_iErreurWSA = GetErreur() ;
                if (!m_bModeSilencieux) ERREUR(MASQUE_ERREUR,"Erreur gethostbyname %d vers [%s]",m_iErreurWSA, szNomMachine) ;
                else ERREUR(MASQUE_DEBUG,"Erreur gethostbyname %d vers [%s]",m_iErreurWSA, szNomMachine) ;
                return false ;
            }

            // Recup de l'adresse
            myaddr.sin_addr.s_addr = ulAddr;
        }
    }

    // Recup des infos sur le service utilisé dans ../etc/services
    myaddr.sin_port = htons(m_sPortDefaut) ;

    // Lecture etc/services
    sp = getservbyname(szNomService, "tcp");
    if (sp == NULL) {
        // Si pas de port par défaut configuré
        if (m_sPortDefaut == 0 ) {
            m_iErreurWSA = GetErreur() ;
            if (!m_bModeSilencieux) ERREUR(MASQUE_ERREUR, "%s/tcp service non trouve %d",szNomService,m_iErreurWSA) ;
            else ERREUR(MASQUE_DEBUG, "%s/tcp service non trouve %d",szNomService,m_iErreurWSA) ;
            return false ;
        }
    }
    else myaddr.sin_port = sp->s_port;

    // Création du socket
    s = socket (AF_INET, SOCK_STREAM, 0);
    if (s == SOCKET_ERROR) {
        m_iErreurWSA = GetErreur() ;
        if (!m_bModeSilencieux) ERREUR(MASQUE_ERREUR, "Création socket impossible %d",m_iErreurWSA);
        else ERREUR(MASQUE_DEBUG, "Création socket impossible %d",m_iErreurWSA);
        return false ;
    }

    // Mise a jour de la structure pour connect
    myaddr.sin_family = AF_INET;

    // Parametrage de la taille des buffers
    int iTaille = SRvMESSAGE_MAX ;
    if (setsockopt( s, SOL_SOCKET, SO_SNDBUF, (const char*)&iTaille,sizeof(iTaille))!=0){
        ERREUR(MASQUE_ERREUR, " %s: operation setsockopt SO_SNDBUF refusee",szNomService);
    }

    if (setsockopt( s, SOL_SOCKET, SO_RCVBUF, (const char*)&iTaille,sizeof(iTaille))!=0){
        ERREUR(MASQUE_ERREUR, " %s: operation setsockopt SO_RCVBUF refusee",szNomService);
    }


    // Détermination de l'@ IP de sortie
    if ((szNomMachineLocale != NULL) && (strlen(szNomMachineLocale))){

        // Mise a jour de la structure pour bind
        myaddrLocale.sin_family = AF_INET;

        // Test si @ IP
        myaddrLocale.sin_addr.s_addr  = inet_addr(szNomMachineLocale) ;

        // Test adresse ou hostname
        if (myaddrLocale.sin_addr.s_addr == htonl(INADDR_NONE)) {

            // Resolution du nom
            ULONG ulAddr = m_pInterfaceTcp->ulGetHostByName(szNomMachine);
            if ( ulAddr == 0) {
                m_iErreurWSA = GetErreur() ;
                if (!m_bModeSilencieux) ERREUR(MASQUE_ERREUR,"Erreur gethostbyname %d pour @ IP de sortie pour [%s]",
                                               m_iErreurWSA, szNomMachineLocale) ;
                else ERREUR(MASQUE_DEBUG,"Erreur gethostbyname %d pour @ IP de sortie pour [%s]",m_iErreurWSA, szNomMachineLocale) ;
                return false ;
            }

            // Recup de l'adresse
            myaddrLocale.sin_addr.s_addr = ulAddr;
        }

        // Recup des infos sur le service utilisé dans ../etc/services
        myaddrLocale.sin_port = htons(0) ;

        // Affectation de l'@ IP de sortie
#ifdef _HPUX_SOURCE
        if ((bind( s ,&myaddrLocale, sizeof(myaddrLocale))) < 0 ) {
#else
        if ((bind( s ,(struct sockaddr*) &myaddrLocale, sizeof(myaddrLocale))) < 0 ) {
#endif
            m_iErreurWSA = GetErreur() ;
            if (!m_bModeSilencieux) ERREUR(MASQUE_ERREUR, " %s: operation bind refusee. Erreur %d sur @ IP de sortie (%s)", szNomService, m_iErreurWSA, szNomMachineLocale);
            else ERREUR(MASQUE_DEBUG, " %s: operation bind refusee. Erreur %d sur @ IP de sortie (%s)", szNomService, m_iErreurWSA, szNomMachineLocale);
            return false ;
        }
    }

    // Prépositionnement de l'erreur
    m_iEtatConnexionEnCours = ETAT_SOCKET_ERREUR ;

    // Mode NON BLOQUANT d'ouverture
    if (bConnectNonBloquant) {
        if (!bNonBloquant()) return false ;

        // Prépositionnement du résultat
        m_iEtatConnexionEnCours = ETAT_SOCKET_EN_COURS;

        // Mise du socket dans le masque d'écriture pour détecter la connexion réelle ou l'échec
        if (m_pInterfaceTcp) {
            m_pInterfaceTcp->bAjouteCanalService(this, TCP_MASQUE_ECRITURE|TCP_MASQUE_EXCEPTION);
        }
    }

    // Tentative de connexion au service
#ifdef _HPUX_SOURCE
    if (connect(s,&myaddr,sizeof(myaddr)) == SOCKET_ERROR) {
#else
    if (connect(s,(struct sockaddr*) &myaddr,sizeof(myaddr)) == SOCKET_ERROR) {
#endif

        m_iErreurWSA = errno;
        if ((bConnectNonBloquant) && (m_iErreurWSA == EINPROGRESS)) {

            ERREUR(MASQUE_DEBUG, "Connect activé, en cours vers IP=[%s] Port=[%hd]",inet_ntoa(myaddr.sin_addr),ntohs(myaddr.sin_port));
            // MAJ Adresses IP & MAC
            vSetAdresses(myaddr, m_szMAC, m_szIP, m_usPORT) ;
            return true;
        }
        else {
            // On est en non bloquant, on restaure l'état
            if (bConnectNonBloquant) {
                m_iEtatConnexionEnCours = ETAT_SOCKET_ERREUR;
                if (m_pInterfaceTcp) {
                    m_pInterfaceTcp->bSupprimeCanalService(this);
                }
            }

            // Fermeture socket
            if (!m_bModeSilencieux) ERREUR(MASQUE_ERREUR, "IP=[%s,%hd] Connect impossible %d - socket=%d", inet_ntoa(myaddr.sin_addr), ntohs(myaddr.sin_port), m_iErreurWSA, s);
            else ERREUR(MASQUE_DEBUG, "IP=[%s,%hd] Connect impossible %d - socket=%d", inet_ntoa(myaddr.sin_addr), ntohs(myaddr.sin_port), m_iErreurWSA, s);
            vClose();

            return false ;
        }
    }

    // Passage le socket en non bloquant après l'établissement
    if (!bNonBloquant()) return false ;

    // Effacement de l'erreur
    m_iEtatConnexionEnCours = ETAT_SOCKET_ETABLI ;

    // MAJ Adresses IP & MAC
    vSetAdresses(myaddr, m_szMAC, m_szIP, m_usPORT) ;

    // Récupération config locale
    int iI = sizeof(myaddrLocale);

    getsockname(s, (struct sockaddr*)&myaddrLocale, (socklen_t*)&iI);

    vSetAdresses(myaddrLocale, m_szMAClocal, m_szIPlocal, m_usPORTlocal) ;

    ERREUR(MASQUE_TRACE,"Connect MAC=[%s] IP=[%s] Port=[%hd] (local MAC=[%s] IP=[%s] Port=[%hd])",
           m_szMAC, m_szIP, m_usPORT, m_szMAClocal, m_szIPlocal, m_usPORTlocal);

    return true ;
}

/*!
********************************************************************************
\brief Fixe le nb max d'éléments par fifos
\param usMaxLec Le nombre maximum de noeuds de lecture
\param usMaxEcr Le nombre maximum de noeuds d'écriture
********************************************************************************
*/
void CSRV_Canal::vSetMaxFifos(USHORT usMaxLec , USHORT usMaxEcr ) {

    m_ffEcriture.m_usMaxNoeud = usMaxEcr ;
    m_ffLecture.m_usMaxNoeud = usMaxLec ;
}

/*!
********************************************************************************
\brief Crée les sections critiques necessaires au MT
\param bLecture Indicateur de création de section critique pour la lecture
\param bEcriture Indicateur de création de section critique pour l'écriture
********************************************************************************
*/
void CSRV_Canal::vEnableMultiThread( BOOL bLecture , BOOL bEcriture ) {

    // Section critique sur l'acces fifo lecture
    if (bLecture) m_ffLecture.vCreateSectionCritique() ;

    // Section critique sur le mecanisme d'ecriture
    if (bEcriture) {
        if (m_pcsEcriture == NULL ) {
            m_pcsEcriture = new CCriticalSection ;
        }
    }
}


/*!
********************************************************************************
\brief Fermeture du socket
********************************************************************************
*/
void CSRV_Canal::vClose() {

    closesocket(s) ;

    // Suppression du canal dans le tableau
//	if (m_pInterfaceTcp) m_pInterfaceTcp->bSupprimeCanalService(this);

    s = INVALID_SOCKET ;
    m_iEtatConnexionEnCours = ETAT_SOCKET_NON_ETABLI ;
}

/*!
********************************************************************************
\brief Gestion Lock et UnLock de la section critique en écriture
\param bLock Indicateur de Lock pour la section critique en écriture
********************************************************************************
*/
void CSRV_Canal::vCSEcriture(BOOL bLock )  {

    // Test existance de la Section critique
    if (m_pcsEcriture == NULL) return ;

    if (bLock)
        m_pcsEcriture->Lock() ;
    else
        m_pcsEcriture->Unlock() ;
}

/*!
********************************************************************************
\brief Log du mesage et fermeture canal
\param iCause La cause de fermeture du canal
\param iTaille La taille des éléments restant
********************************************************************************
*/
void CSRV_Canal::vTraiteException(int iCause, int iTaille) {
    ERREUR(MASQUE_ERREUR_CRITIQUE,"Exception %d %s Taille %d",iCause, (iCause == SATURATION_ECRITURE) ? "ECRITURE" : "LECTURE", iTaille) ;
    vFermeCanal() ;
}


/*!
********************************************************************************
\brief Récuperation des adresses IP et MAC du socket et mise à jour des variables m_szMAC et m_szIP
\param adresse L'adresse de connexion
\param szMAC Adresse MAC
\param szIP Adresse IP
\param usPORT (OUT) Numéro de port
********************************************************************************
*/
void CSRV_Canal::vSetAdresses(struct sockaddr_in adresse, LPSTR szMAC, LPSTR szIP, USHORT &usPORT) {

    BYTE* pAdr = NULL ;

    // Conversion adresse IP
    char * sz = inet_ntoa(adresse.sin_addr) ;
    if (sz) strncpy(szIP, sz, MAX_ADRESSE) ;

    // Mémorisation port
    usPORT = ntohs(adresse.sin_port);

    // Gestion MAC ADRESSE
    szMAC[0]=0;

    struct arpreq myarp  ;
    struct sockaddr_in myaddr;

    errno = 0;

    myaddr.sin_family = AF_INET ;
    myaddr.sin_addr	  = adresse.sin_addr;

    memset(&myarp,0,sizeof(myarp)) ;
    memcpy(&myarp.arp_pa, &myaddr, sizeof myarp.arp_pa);


    char			szBuff[2000] ;
    struct ifconf	sConf ;

    // Initialisation sConf
    sConf.ifc_len = sizeof(szBuff) ;
    sConf.ifc_buf = szBuff ;

    // pRep pointe sur szBuff
    ifreq*	pInfo =(ifreq*) szBuff;

    // Recup de la liste des cartes ethernet
    if (ioctl(s, SIOCGIFCONF, &sConf)  != -1) {

        int iNb = sConf.ifc_len / sizeof(ifreq) ;

        // 0 == loopback
        int i = 1 ;
        do {

            strcpy(myarp.arp_dev, pInfo[i++].ifr_ifrn.ifrn_name);

            if (ioctl(s, SIOCGARP, &myarp) != -1)
                pAdr = (BYTE*)&myarp.arp_ha.sa_data[0];


        } while ((pAdr == NULL) && ( i < iNb)) ;
    }

    // Mise a jour chaine si ok
    if (pAdr) sprintf(szMAC,"%02X:%02X:%02X:%02X:%02X:%02X",pAdr[0],pAdr[1],pAdr[2],pAdr[3],pAdr[4],pAdr[5]);
}


/*!
********************************************************************************
\brief Récupération du numéro de port à utiliser
\param szNomMachine Nom de la machine
\param szNomService Nom du service
\param sAddr (OUT) Socket à initialiser avec le port
\return Renvoi vrai si le numéro de port a bien été récupéré, renvoi faux sinon
********************************************************************************
*/
BOOL CSRV_Canal::bInitSockAddr(LPCSTR szNomMachine, LPCSTR szNomService, struct sockaddr_in & sAddr) {
    char*	pcPort = NULL ;
    char	szMachine[250] ;
    ULONG	ulAddr;

    // affectation du no de port par defaut
    int iNumPort = m_sPortDefaut ;

    szMachine[0] = 0 ;
    if (szNomMachine) strncpy(szMachine,szNomMachine,250) ;

    // Gestion du nom de la machine
    if (strlen(szMachine)){

        // Test de la présence du no de port
        pcPort = strchr(szMachine,':') ;
        if (pcPort) *pcPort++= 0 ;

        // Test si @ IP
        if (strlen(szMachine) && stricmp(szMachine,"localhost")) {

            sAddr.sin_addr.s_addr  = inet_addr(szMachine) ;

            // Test adresse ou hostname
            if (sAddr.sin_addr.s_addr == htonl(INADDR_NONE)) {

                // Recup adresse IP de la machine
                ulAddr =  CSRV_Tcp::ulGetHostByName(szMachine);
                if ( ulAddr == 0) {
                    m_iErreurWSA = GetErreur() ;
                    ERREUR(MASQUE_ERREUR,"Erreur ulGetHostByName %s : %d",szMachine, m_iErreurWSA) ;
                    return false ;
                }

                sAddr.sin_addr.s_addr = ulAddr ;
            }
        }

    }


    // On vérifie si un numéro de port a été fourni avec le nom de la machine
    if (pcPort) {

        // Cas où le numéro de port a été fourni avec le nom de la machine
        // Récupération du numéro de port dans la chaine
        iNumPort = atoi(pcPort);
    }
    else {

        if (szNomService) {
            // Recherche du numéro de port dans le fichier XML
            CRegistryXML oRegXML;
            oRegXML.vAddFile(FICHIER_REGISTRY_MACHINE);
            char szNomClefNumPort[200];
            sprintf(szNomClefNumPort, "PortTCP.%s", szNomService);
            oRegXML.bLireClef(szNomClefNumPort, iNumPort, m_sPortDefaut);
        }
        else
            iNumPort = 0 ;
    }

    // Affectation no de port
    sAddr.sin_port = htons(iNumPort) ;


    return true;
}
