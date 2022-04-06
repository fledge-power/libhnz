#ifndef CHNZLIB
#define CHNZLIB

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/timeb.h>
#include "hnz_fifo.h"
#include "TcpConnexion.h"
#include "../hal/inc/automate.h"

#define	TAILLE_MAX        1024
#define TAILLE_ENTETE        6
#define	DLE_HNZ  0x87
#define	SUB_HNZ  0xA7
#define	SEND_HNZ 0x0D
#define LONGUEUR_CRC_MOT 2
#define EVT_VOIE_TRAME_HNZ 21
#define	POLYNOME_1 0x8408 // Polynome pour calcul CRC


#define	SARM					0x0F
#define	UA						0x63
#define	RR						0x01

#define TM4                     0x02
#define TSCE                    0x0B
#define TSCG                    0x16
#define TMN                     0x0C


class TcpConnexion;
class CAutomateHNZ;

typedef struct {
    int       iEvt ;                  ///< Evénement associé à la trame
    int       iErreur;                ///< Code utilisé par le système de messages manuelles, pour emmettre un message éronné
    int usLgBuffer;
    unsigned char aubTrame[TAILLE_MAX];
    uint64_t ullTpsEmission;
} MSG_TRAME  ;

typedef union {
    unsigned int   uiVal ;
    unsigned char  bVal[4] ;
    unsigned int usVal[2] ;
} VCRC ;


class VoieHNZ {

public:
    VoieHNZ();
    int iConnecte();
    MSG_TRAME* creerTrame();
    void vInitEnteteTrameData( MSG_TRAME *pTrame, bool bMaj );
    void vEnvoiTrameVersHnz();
    void vEnvoiTrameVersHnz( MSG_TRAME *pTrame );
    VCRC calculCRCMot(MSG_TRAME *pTrame, unsigned int uiPolynome);
    int  iVerifCRCMot(MSG_TRAME *pTrame, unsigned int uiPolynome);
    void vSetCRCMot(MSG_TRAME *pTrame, unsigned int uiPolynome);
    bool bAjouteCar(unsigned char ub);
    bool vTraiteEvt(int iEvt);
    void setClient(TcpConnexion* client);
    void setServer(TcpConnexion* server);
    void vGereAutomate();
    MSG_TRAME* trDepileTrame();
    void vEmpileTrame(MSG_TRAME *ptrame);
    void iConnecteSlave();
    void iConnecteMaster();
    void setAutomate(CAutomateHNZ *automate);
    TcpConnexion* connslave;
    TcpConnexion* connMaster;
    CAutomate* m_AutomateHNZ;
    int m_uiLus;
    unsigned char m_abBuffer [TAILLE_MAX+TAILLE_ENTETE] ;

private:
    bool m_bTransparenceEnCours;
    bool m_bEmissionRecente;


    CThreadMQ<MSG_TRAME>	m_ffTramesAEnvoyer ;	///< Fifo trames à envoyer
    CThreadMQ<MSG_TRAME>	m_ffTramesAttAcq ;		///< Fifo trames en attentes d'acquittement
    CThreadMQ<MSG_TRAME>	m_ffTramesARepeter ;	///< Fifo trames à répéter
    CThreadMQ<MSG_TRAME>	m_ffTramesRecus ;       ///< Fifo trames reçus

    friend class TcpConnexion;
};




#endif 
