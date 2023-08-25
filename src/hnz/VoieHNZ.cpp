#include <thread>
#include <sstream>
#include <iomanip>

#include "../inc/VoieHNZ.h"

VoieHNZ::VoieHNZ() {
  m_bTransparenceEnCours = false;
  m_uiLus = 0;
  m_AutomateHNZ = new CAutomate();
  stop_flag = false;
}

void VoieHNZ::iConnecteSlave() {
  // Si on a une classe d'attente associe, c'est elle qui fait le CONNECT
  // (ESCLAVE)
  // if (m_pVoiePendante) return m_pVoiePendante->iConnecte();
  // return socketTcp.iTCPConnecte() ;
  connslave->iTCPConnecteServeur();
}

void VoieHNZ::setServer(TcpConnexion *server) { connslave = server; }

void VoieHNZ::setClient(TcpConnexion *client) { connMaster = client; }

void VoieHNZ::iConnecteMaster() { connMaster->iTCPConnecteClient(); }

void VoieHNZ::vInitEnteteTrameData(MSG_TRAME *pTrame, bool bMaj) {
  MSG_TRAME *TrameTmp = new MSG_TRAME;

  if (!bMaj) {
    // Pour une nouvelle trame, il faut laisser la place pour les 2 premiers
    // octets
    memcpy(TrameTmp->aubTrame + 2, pTrame->aubTrame, pTrame->usLgBuffer);
    pTrame->usLgBuffer += 2;
  } else {
    // Pour une mise  jour les 2 premiers octets vont tre cras par lInita suite
    memcpy(TrameTmp->aubTrame, pTrame->aubTrame, pTrame->usLgBuffer);
  }

  // vLogMessageVoie( DUMP_DEBUG, TypeTrace::Autre, 0, "HNZ : Envoi Trame I -
  // NR=%d - NS=%d", m_iNoTrameAttEmission, m_iNoSequenceEnCours ) ;
  memcpy(pTrame->aubTrame, TrameTmp->aubTrame, pTrame->usLgBuffer);
}

/*!
*************************************************************************************
\brief Envoi un trame HNZ par TCP en codant la transparence
\param pTrame Trame  envoyer
****************************************************************************************/
void VoieHNZ::vEnvoiTrameVersHnz(MSG_TRAME *pTrame) {
  if (!pTrame) return;
  // printf("VoieHNZ::vEnvoiTrameVersHnz - socket %d - Frame added to stack [%s]\n", connslave->socketfd, frameToStr(pTrame).c_str());
  m_ffTramesAEnvoyer.bEmpile(pTrame);
  // Verifie qu'il y a bien un timer en attente, et s'il n'y en a pas, envoie immédiatement
  if (m_bEmissionRecente && (m_AutomateHNZ->iGetNbTimers() == 0)) {
    m_bEmissionRecente = false;
  }
  // Si il n'y a pas eu d'mission rcente, on envoie, sinon, un timer dclenchera
  // l'mission plus tard.
  if (!m_bEmissionRecente) vEnvoiTrameVersHnz();
}

void VoieHNZ::vEnvoiTrameVersHnz() {
  MSG_TRAME *pTrame = m_ffTramesAEnvoyer.pDepile();
  if (!pTrame) {
    m_bEmissionRecente = false;
    return;
  }
  // printf("VoieHNZ::vEnvoiTrameVersHnz - socket %d - Frame taken from stack [%s]\n", connslave->socketfd, frameToStr(pTrame).c_str());

  unsigned char bTrame[2 * TAILLE_MAX];

  memset(bTrame, 0, 2 * TAILLE_MAX);

  int iIndex = 0;
  for (int i = 0; i < (int)pTrame->usLgBuffer; i++) {
    if (pTrame->aubTrame[i] == SEND_HNZ) {
      bTrame[iIndex++] = DLE_HNZ;
      bTrame[iIndex++] = SUB_HNZ;
    } else if (pTrame->aubTrame[i] == DLE_HNZ) {
      bTrame[iIndex++] = DLE_HNZ;
      bTrame[iIndex++] = DLE_HNZ;
    } else
      bTrame[iIndex++] = pTrame->aubTrame[i];
  }

  // Ajout du caractre terminateur de la trame
  bTrame[iIndex++] = SEND_HNZ;

  connslave->vSend(bTrame, iIndex);
  // printf("Message envoyé : %s\n", bTrame);

  m_AutomateHNZ->vAddTimer(DELAI_INTER_TRAME_MS, /*EVT_EMISSION*/ 1, NULL);

  m_bEmissionRecente = true;

  // delete(pTrame);
}

/*!
*************************************************************************************
\brief Assemblage de trames HNZ
\param ub Caractre  assembler
\return Vrai si fin de trame
****************************************************************************************/
bool VoieHNZ::bAjouteCar(unsigned char ub) {
  // Dtection fin de trame
  if (ub == SEND_HNZ) {
    return true;
  }

  // if (ub == '\0') {
  //     return false;
  // }

  if (m_bTransparenceEnCours) {
    if (ub == DLE_HNZ)
      ub = DLE_HNZ;
    else if (ub == SUB_HNZ)
      ub = SEND_HNZ;
    else {
      ub = 0;
    }
    m_bTransparenceEnCours = false;
  } else if (ub == DLE_HNZ) {
    m_bTransparenceEnCours = true;
    return false;
  }

  // Test taille MAX
  if (m_uiLus >= TAILLE_MAX + TAILLE_ENTETE) {
    m_uiLus = 0;
  }

  m_abBuffer[m_uiLus++] = ub;
  return false;
}

void VoieHNZ::vGereAutomate() {
  printf("VoieHNZ::vGereAutomate - socket %d - Timers management thread started\n", connslave->socketfd);

  while (!stop_flag) {
    void *pv;
    int iEvtTimer;
    bool timer = m_AutomateHNZ->bGetNextTimer(iEvtTimer, pv);
    if (timer) {
      // printf("VoieHNZ::vGereAutomate - socket %d - Processing event of type %d\n", connslave->socketfd, iEvtTimer);
      switch (iEvtTimer) {
        case 1:
          vEnvoiTrameVersHnz();
          break;
        case 2:
          break;
      }
    }
    // Get the time until next timer expiration
    uint delay = m_AutomateHNZ->uiGetTimeOut();
    if (delay == 0x7FFFFFF) {
      //printf("VoieHNZ::vGereAutomate - socket %d - No timer in list\n", connslave->socketfd);
      // Use 50ms as default delay as this is the minimum time ever set for a new timer
      delay = DELAI_INTER_TRAME_MS;
    }
    //printf("VoieHNZ::vGereAutomate - socket %d - Waiting for %d ms\n", connslave->socketfd, delay);
    std::this_thread::sleep_for(std::chrono::milliseconds{delay});
  }

  printf("VoieHNZ::vGereAutomate - socket %d - Timers management thread stopped\n", connslave->socketfd);
}

/*!
*************************************************************************************
\brief Empile une trame dans la fifo de l'quipement
\param pTrame Pointeur sur la trame  empiler
\return Si ok ==> envoi de la trame
****************************************************************************************/

/*!
 **************************************************************************************
 * \brief calculCRCMot calcul le CRC/FCS d'un message
 * \param pTrame     la trame, dont la longueur inclus le CRC
 * \param uiPolynome le polynome utilis pour le calcul du CRC
 * \return le CRC de la trame
 ****************************************************************************************/
VCRC VoieHNZ::calculCRCMot(MSG_TRAME *pTrame, unsigned int uiPolynome) {
  VCRC vcrc;
  int iw, jw, iLg = pTrame->usLgBuffer;

  vcrc.uiVal = 0;
  vcrc.bVal[0] = ~pTrame->aubTrame[0];
  vcrc.bVal[1] = ~pTrame->aubTrame[1];

  for (jw = 2; jw < iLg - 2; jw++) {
    vcrc.bVal[2] = pTrame->aubTrame[jw];
    for (iw = 0; iw < 8; iw++) {
      if ((vcrc.uiVal & 0x00000001) != 0)
        vcrc.uiVal = (vcrc.uiVal >> 1) ^ uiPolynome;
      else
        vcrc.uiVal = (vcrc.uiVal >> 1);
    }
  }

  for (jw = 0; jw < 2; jw++) {
    vcrc.bVal[2] = 0;
    for (iw = 0; iw < 8; iw++) {
      if ((vcrc.uiVal & 0x00000001) != 0)
        vcrc.uiVal = (vcrc.uiVal >> 1) ^ uiPolynome;
      else
        vcrc.uiVal = (vcrc.uiVal >> 1);
    }
  }
  return vcrc;
}

/*!
*************************************************************************************
\brief Verification du CRC/FCS du message recu
\param pTrame Trame  verifier
\param uiPolynome Polynome
\return Difference entre CRC calcule et CRC present dans la trame (soit '0' si
identiques)
****************************************************************************************/
int VoieHNZ::iVerifCRCMot(MSG_TRAME *pTrame, unsigned int uiPolynome) {
  int iRet = 0;
  int iLg = pTrame->usLgBuffer;
  VCRC vcrc = calculCRCMot(pTrame, uiPolynome);
  if ((vcrc.bVal[0] == pTrame->aubTrame[iLg - 2]) &&
      (vcrc.bVal[1] == pTrame->aubTrame[iLg - 1])) {
    iRet = 1;
  } else {
    vcrc.usVal[0] = ~vcrc.usVal[0];
    if ((vcrc.bVal[0] != pTrame->aubTrame[iLg - 2]) ||
        (vcrc.bVal[1] != pTrame->aubTrame[iLg - 1]))
      iRet = -1;
  }
  return iRet;
}

/*!
*************************************************************************************
\brief Ajoute CRC en fin de trame
\param	pTrame Donnes a traiter
\param	iLg Longueur de la trame  traiter
\param  uiPoly Polynome associe au CRC
\n Le tampon 'trame' doit avoir la taille suffisante pour recevoir le CRC (2
octets)
****************************************************************************************/
void VoieHNZ::vSetCRCMot(MSG_TRAME *pTrame, unsigned int uiPolynome) {
  // pour calculer le CRC, on donne  la trame sa taille finale, donc avec le CRC
  pTrame->usLgBuffer += LONGUEUR_CRC_MOT;
  VCRC vcrc = calculCRCMot(pTrame, uiPolynome);
  // Positionnement du CRC
  vcrc.usVal[0] = ~vcrc.usVal[0];
  pTrame->aubTrame[pTrame->usLgBuffer - 2] = vcrc.bVal[0];
  pTrame->aubTrame[pTrame->usLgBuffer - 1] = vcrc.bVal[1];
}

/*!
*************************************************************************************
\brief Traite une trame complte. Si ok ==> envoi de la trame
\param iEvt Evenement  traiter
****************************************************************************************/
bool VoieHNZ::vTraiteEvt(int iEvt) {
  // Allocation
  MSG_TRAME *pTrame = new MSG_TRAME;
  memset(pTrame, 0, sizeof(MSG_TRAME));

  // Mise a jour de la trame
  // pTrame->pVoie		= this;
  pTrame->iEvt = iEvt;
  pTrame->usLgBuffer = 0;
  // pTrame->iSession    = m_iSession ;

  if (iEvt == EVT_VOIE_TRAME_HNZ) {
    pTrame->usLgBuffer = m_uiLus;
    // Si c'est une trame on l'empile

    memcpy(pTrame->aubTrame, m_abBuffer, m_uiLus);

    // Dump
    // vDumpTrame(m_abBuffer, m_uiLus, "Reception  <<") ;

    /// Envoi Evt pour connexion vEvtStats(EVT_STAT_RECEPTION_TRAME_A+m_iVoie)
    // m_pLigneHNZ->vEvtStats(EVT_STAT_RECEPTION_TRAME_A + (m_iVoie %
    // NB_EQUIPEMENT_VOIES)) ;
  }

  // Empile la trame
  vEmpileTrame(pTrame);

  // Raz buffer pour la prochaine trame
  memset(m_abBuffer, 0, sizeof(m_abBuffer));
  m_uiLus = 0;

  return true;
}

void VoieHNZ::vEmpileTrame(MSG_TRAME *ptrame) {
  m_ffTramesRecus.bEmpile(ptrame);
}

MSG_TRAME *VoieHNZ::trDepileTrame() { return m_ffTramesRecus.pDepile(); }

/**
 * Helper method to convert payload into something readable for logs.
 */
std::string VoieHNZ::frameToStr(MSG_TRAME *pTrame) {
  if (pTrame == nullptr) {
    return "";
  }
  int len = pTrame->usLgBuffer;
  unsigned char* data = pTrame->aubTrame;
  std::stringstream stream;
  for (int i = 0; i < len; i++) {
    stream << std::setfill ('0') << std::setw(2) << std::hex << static_cast<unsigned int>(data[i]);
    if (i < len - 1) stream << " ";
  }
  return stream.str();
}