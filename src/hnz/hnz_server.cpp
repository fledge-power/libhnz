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

std::thread HNZServer::launchAutomate() {
    VoieHNZ* taskPtr = m_pVoie;
    std::thread t(&VoieHNZ::vGereAutomate, taskPtr);
    return t;
}

void HNZServer::stop() {
    m_ThreadAutomate.join();
}


void HNZServer::start(int port) {
    m_pConn->iTCPConnecteServeur(port);
    m_ThreadAutomate = launchAutomate();

}

void HNZServer::start() {
    m_pConn->iTCPConnecteServeur();
    m_ThreadAutomate = launchAutomate();
}

void HNZServer::addMsgToFr(MSG_TRAME *trame, unsigned char* msg) {
    char* tmpmsg = (char*) msg;
    memcpy(trame->aubTrame,msg, strlen(tmpmsg));
    trame->usLgBuffer = strlen(tmpmsg);
}

void HNZServer::addMsgToFr(MSG_TRAME *trame, unsigned char* msg, int msgSize) {
    memcpy(trame->aubTrame, msg, msgSize);
    trame->usLgBuffer = msgSize;
}


void HNZServer::createAndSendFr(unsigned char* msg) {
    MSG_TRAME* pTrame = new MSG_TRAME;
    addMsgToFr(pTrame,msg);
    sendFr(pTrame);
}

void HNZServer::receiveFr() {
      while(1) {

        bool trameRecue = m_pConn->vAssembleTrame();
        if (trameRecue) {
            //m_pTrameRecu = m_pVoie->trDepileTrame();
            printf("Message reçu : %s\n",m_pVoie->trDepileTrame()->aubTrame);
        }

    }
}

unsigned char* HNZServer::receiveData() {
    bool trameRecue = m_pConn->vAssembleTrame();
    if (trameRecue) {
        return (m_pVoie->trDepileTrame()->aubTrame);
    }
    return nullptr;
}

void HNZServer::sendFr(MSG_TRAME* trame){
    m_pVoie->vEnvoiTrameVersHnz(trame);
}