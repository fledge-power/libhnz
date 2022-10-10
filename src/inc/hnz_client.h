//
// Created by estelle on 30/03/2021.
//

#ifndef PERSO_HNZ_HNZ_CLIENT_H
#define PERSO_HNZ_HNZ_CLIENT_H
#define POLYNOME_1 0x8408  // Polynome pour calcul CRC

#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>

#include <atomic>
#include <string>
#include <thread>

#include "../hal/inc/automate.h"
#include "VoieHNZ.h"

class HNZClient {
 public:
  HNZClient();
  void sendFr(MSG_TRAME *trame);
  void createAndSendFr(unsigned char *msg);
  void createAndSendFr(unsigned char addr, unsigned char *msg, int msgSize);
  int connect_Server(const char *adresse, int port);
  void connect_Server();
  void addMsgToFr(MSG_TRAME *trame, unsigned char *msg);
  void addMsgToFr(MSG_TRAME *trame, unsigned char *msg, int msgSize);
  void setCRC(MSG_TRAME *trame);
  bool checkCRC(MSG_TRAME *trame);
  // void receiveFr();
  unsigned char *receiveData();
  MSG_TRAME *receiveFr();
  MSG_TRAME *getFr();
  void stop();
  bool is_connected();
  std::thread launchAutomate();

 private:
  VoieHNZ *m_pVoie;
  TcpConnexion *m_pConn;
  MSG_TRAME *m_pTrameRecu;
  std::thread m_ThreadAutomate;
  bool started = false;
};

#endif  // PERSO_HNZ_HNZ_CLIENT_H
