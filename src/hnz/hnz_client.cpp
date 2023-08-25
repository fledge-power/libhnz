//
// Created by estelle on 30/03/2021.
//

#include "../inc/hnz_client.h"

HNZClient::HNZClient() {
  m_pVoie = new VoieHNZ();
  m_pConn = new TcpConnexion(m_pVoie);
  m_pVoie->setServer(m_pConn);
  m_pVoie->setClient(m_pConn);
}

HNZClient::~HNZClient() {
  stop();
  delete m_pVoie;
  delete m_pConn;
}

std::thread HNZClient::launchAutomate() {
  m_pVoie->stop_flag = false;
  std::thread t(&VoieHNZ::vGereAutomate, m_pVoie);
  return t;
}

void HNZClient::stop() {
  if (!m_pVoie->stop_flag) {
    // Mark as stopped
    m_pVoie->stop_flag = true;
    // Stopping automate
    if (m_ThreadAutomate.joinable()) {
      m_ThreadAutomate.join();
    }
    // Stopping socket...
    m_pConn->stop();
  }
}

int HNZClient::connect_Server(const char* adresse, int port, long long int recvTimeoutUs) {
  // If a connection was already established, stop it
  stop();
  // Open a new connection
  int con_opened = m_pConn->iTCPConnecteClient(adresse, port, recvTimeoutUs);
  if (con_opened == 0) {
    if (!m_ThreadAutomate.joinable()) {
      m_ThreadAutomate = launchAutomate();
    }
  }
  return con_opened;
}

// void HNZClient::receiveFr() {
//       while(1) {

//         bool trameRecue = m_pConn->vAssembleTrame();
//         if (trameRecue) {
//             //m_pTrameRecu = m_pVoie->trDepileTrame();
//             printf("Message reçu : %s\n",m_pVoie->trDepileTrame()->aubTrame);
//         }

//     }
// }

unsigned char* HNZClient::receiveData() {
  bool trameRecue = m_pConn->vAssembleTrame();
  if (trameRecue) {
    return (m_pVoie->trDepileTrame()->aubTrame);
  }
  return nullptr;
}

MSG_TRAME* HNZClient::receiveFr() {
  bool trameRecue = m_pConn->vAssembleTrame();
  if (trameRecue) {
    return (m_pVoie->trDepileTrame());
  }
  return nullptr;
}

MSG_TRAME* HNZClient::getFr() { return m_pTrameRecu; }

void HNZClient::connect_Server() { m_pConn->iTCPConnecteClient(); }

void HNZClient::addMsgToFr(MSG_TRAME* trame, unsigned char* msg) {
  memcpy(trame->aubTrame, msg, sizeof(msg) + 1);
  trame->usLgBuffer = sizeof(msg) + 1;
}

void HNZClient::addMsgToFr(MSG_TRAME* trame, unsigned char* msg, int msgSize) {
  memcpy(trame->aubTrame, msg, msgSize);
  trame->usLgBuffer = msgSize;
}

void HNZClient::setCRC(MSG_TRAME* trame) {
  m_pVoie->vSetCRCMot(trame, POLYNOME_1);
}

void HNZClient::createAndSendFr(unsigned char* msg) {
  MSG_TRAME* pTrame = new MSG_TRAME;
  addMsgToFr(pTrame, msg);
  setCRC(pTrame);
  sendFr(pTrame);
}

void HNZClient::createAndSendFr(unsigned char addr, unsigned char* msg,
                                int msgSize) {
  MSG_TRAME* pTrame = new MSG_TRAME;
  unsigned char msgWithAddr[msgSize + 1];
  // Add address
  msgWithAddr[0] = addr;
  // Add message
  memcpy(msgWithAddr + 1, msg, msgSize);
  addMsgToFr(pTrame, msgWithAddr, sizeof(msgWithAddr));
  setCRC(pTrame);
  sendFr(pTrame);
}

void HNZClient::sendFr(MSG_TRAME* trame) { m_pVoie->vEnvoiTrameVersHnz(trame); }

bool HNZClient::is_connected() { return m_pConn->is_connected(); }

bool HNZClient::checkCRC(MSG_TRAME* trame) {
  int dist = m_pVoie->iVerifCRCMot(trame, POLYNOME_1);
  return (dist == 0);
}