//
// Created by estelle on 30/03/2021.
//

#ifndef PERSO_HNZ_HNZ_SLAVE_H
#define PERSO_HNZ_HNZ_SLAVE_H


#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include "../hal/inc/automate.h"
#include "TcpConnexion.h"
#include "VoieHNZ.h"
#include <thread>

class HNZServer {
public:
    HNZServer();
    void start(int port);
    void start();
    void addMsgToFr(MSG_TRAME *trame, unsigned char* msg);
    void addMsgToFr(MSG_TRAME *trame, unsigned char *msg, int msgSize);
    void createAndSendFr(unsigned char* msg);
    void sendFr(MSG_TRAME *trame);
    void receiveFr();
    unsigned char* receiveData();
    MSG_TRAME* getFr();
    std::thread launchAutomate();
    void stop();


private:
    TcpConnexion* m_pConn;
    VoieHNZ* m_pVoie;
    MSG_TRAME* m_pTrameRecu;
    std::thread m_ThreadAutomate;
};


#endif //PERSO_HNZ_HNZ_SLAVE_H
