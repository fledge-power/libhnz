#ifndef CPORTCP_H
#define CPORTCP_H

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "VoieHNZ.h"
// port par défaut
#define PORT 1234
#define ADRESSE "127.0.0.1"
#define RECV_TIMEOUT 100000 // 100ms

// portTCP

class VoieHNZ;

/// Classe utilitaire pour l'entrée en la sortie (automatique) de section
/// critique
class CCSLock {
 public:
  CCSLock(CCriticalSection *pLock) {
    m_pLock = pLock;
    m_pLock->Lock();
  }

  ~CCSLock() { m_pLock->Unlock(); }

 private:
  CCriticalSection *m_pLock;
};

class TcpConnexion {
 public:
  TcpConnexion();
  TcpConnexion(VoieHNZ *voie);
  void vSend(unsigned char *m_abBuffer, int buffSize);
  void vRecv();
  int iTCPConnecteClient();
  int iTCPConnecteClient(const char *adresse, int port, long long int recvTimeoutUs);
  void iTCPConnecteServeur();
  void iTCPConnecteServeur(int port);
  bool vAssembleTrame();
  bool is_connected();
  void stop();
  int server_fd = -1;
  int socketfd = -1;
  VoieHNZ *m_pVoie = nullptr;
  bool loop = true;

 private:
  CCriticalSection *m_pcsAcces = nullptr;
};

#endif