//
// Created by estelle on 30/03/2021.
//

#ifndef PERSO_HNZ_HNZ_SLAVE_H
#define PERSO_HNZ_HNZ_SLAVE_H

#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>

#include <atomic>
#include <thread>

#include "../hal/inc/automate.h"
#include "TcpConnexion.h"
#include "VoieHNZ.h"

class HNZServer {
 public:
  HNZServer();
  ~HNZServer();
  void start(int port);
  void start();
  void addMsgToFr(MSG_TRAME* trame, unsigned char* msg);
  void addMsgToFr(MSG_TRAME* trame, unsigned char* msg, int msgSize);
  void createAndSendFr(unsigned char* msg);
  void createAndSendFr(unsigned char addr, unsigned char* msg, int msgSize);
  void sendFr(MSG_TRAME* trame);
  // void receiveFr();
  MSG_TRAME* receiveFr();
  unsigned char* receiveData();
  MSG_TRAME* getFr();
  std::thread launchAutomate();
  void stop();
  void setCRC(MSG_TRAME* trame);
  bool checkCRC(MSG_TRAME* trame);
  bool isConnected();

 private:
  TcpConnexion* m_pConn;
  VoieHNZ* m_pVoie;
  MSG_TRAME* m_pTrameRecu;
  std::thread m_ThreadAutomate;
};

#endif  // PERSO_HNZ_HNZ_SLAVE_H
