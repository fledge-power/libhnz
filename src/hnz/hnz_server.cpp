//
// Created by estelle on 30/03/2021.
//

#include "../inc/hnz_server.h"

HNZServer::HNZServer() {
  m_pVoie = new VoieHNZ();
  m_pConn = new TcpConnexion(m_pVoie);
  m_pVoie->setServer(m_pConn);
  m_pVoie->setClient(m_pConn);
}

HNZServer::~HNZServer() {
  stop();
  delete m_pVoie;
  delete m_pConn;
}

std::thread HNZServer::launchAutomate() {
  m_pVoie->stop_flag = false;
  std::thread t(&VoieHNZ::vGereAutomate, m_pVoie);
  return t;
}

void HNZServer::stop() {
  if (!m_pVoie->stop_flag) {
    m_pVoie->stop_flag = true;
    // Stopping automate
    if (m_ThreadAutomate.joinable()) {
      m_ThreadAutomate.join();
    }
    // Stopping socket...
    m_pConn->stop();
  }
}

void HNZServer::start(int port) {
  m_pConn->iTCPConnecteServeur(port);
  if (!m_ThreadAutomate.joinable()) {
    m_ThreadAutomate = launchAutomate();
  }
}

void HNZServer::start() {
  m_pConn->iTCPConnecteServeur();
  if (!m_ThreadAutomate.joinable()) {
    m_ThreadAutomate = launchAutomate();
  }
}

void HNZServer::addMsgToFr(MSG_TRAME* trame, unsigned char* msg) {
  char* tmpmsg = (char*)msg;
  memcpy(trame->aubTrame, msg, strlen(tmpmsg));
  trame->usLgBuffer = strlen(tmpmsg);
}

void HNZServer::addMsgToFr(MSG_TRAME* trame, unsigned char* msg, int msgSize) {
  memcpy(trame->aubTrame, msg, msgSize);
  trame->usLgBuffer = msgSize;
}

void HNZServer::createAndSendFr(unsigned char* msg) {
  MSG_TRAME* pTrame = new MSG_TRAME;
  addMsgToFr(pTrame, msg);
  sendFr(pTrame);
}

void HNZServer::setCRC(MSG_TRAME* trame) {
  m_pVoie->vSetCRCMot(trame, POLYNOME_1);
}

void HNZServer::createAndSendFr(unsigned char addr, unsigned char* msg,
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

// void HNZServer::receiveFr() {
//   if (!m_pVoie->stop_flag) {
//     while (1) {
//       bool trameRecue = m_pConn->vAssembleTrame();
//       if (trameRecue) {
//         // m_pTrameRecu = m_pVoie->trDepileTrame();
//         printf("Message reçu : %s\n", m_pVoie->trDepileTrame()->aubTrame);
//       }
//     }
//   }
// }

MSG_TRAME* HNZServer::receiveFr() {
  if (!m_pVoie->stop_flag) {
    bool trameRecue = m_pConn->vAssembleTrame();
    if (trameRecue) {
      return (m_pVoie->trDepileTrame());
    }
  }
  return nullptr;
}

unsigned char* HNZServer::receiveData() {
  if (!m_pVoie->stop_flag) {
    bool trameRecue = m_pConn->vAssembleTrame();
    if (trameRecue) {
      return (m_pVoie->trDepileTrame()->aubTrame);
    }
  }
  return nullptr;
}

void HNZServer::sendFr(MSG_TRAME* trame) { m_pVoie->vEnvoiTrameVersHnz(trame); }

bool HNZServer::isConnected() { return m_pConn->is_connected(); }

bool HNZServer::checkCRC(MSG_TRAME* trame) {
  int dist = m_pVoie->iVerifCRCMot(trame, POLYNOME_1);
  return (dist == 0);
}