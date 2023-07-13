#include "../inc/TcpConnexion.h"

TcpConnexion::TcpConnexion() {}

TcpConnexion::TcpConnexion(VoieHNZ *voie) { m_pVoie = voie; }

int TcpConnexion::iTCPConnecteClient() {
  return iTCPConnecteClient(ADRESSE, PORT, RECV_TIMEOUT);
}

int TcpConnexion::iTCPConnecteClient(const char *adresse, int port, long long int recvTimeoutUs) {
  struct sockaddr_in serv_addr;
  printf("TcpConnexion::iTCPConnecteClient - Connection address: %s\n", adresse);

  if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("TcpConnexion::iTCPConnecteClient - Error in socket: %s\n", strerror(errno));
    return -1;
  }

  // Set timeout for recv() calls to avoid busy loop while waiting for messages
  struct timeval tv;
  tv.tv_sec = static_cast<time_t>(recvTimeoutUs / 1000000);
  tv.tv_usec = static_cast<long int>(recvTimeoutUs - (tv.tv_sec * 1000000));
  if (setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv)) {
    printf("TcpConnexion::iTCPConnecteClient - Error in setsockopt: %s\n", strerror(errno));
  }

  serv_addr.sin_family = AF_INET;
  if (port <= 0) {
    port = PORT;
  }

  if ((!strcmp(adresse, "")) || adresse == NULL) {
    adresse = ADRESSE;
  }

  serv_addr.sin_port = htons(port);

  // Convert IPv4 and IPv6 addresses from text to binary form
  if (inet_pton(AF_INET, adresse, &serv_addr.sin_addr) <= 0) {
    printf("TcpConnexion::iTCPConnecteClient - Error in inet_pton: %s\n", strerror(errno));
    return -1;
  }

  if (connect(socketfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    printf("TcpConnexion::iTCPConnecteClient - Error in connect: %s\n", strerror(errno));
    return -1;
  }
  printf("TcpConnexion::iTCPConnecteClient - Connection established\n");
  return 0;
}

void TcpConnexion::iTCPConnecteServeur() { iTCPConnecteServeur(PORT); }

void TcpConnexion::iTCPConnecteServeur(int port) {
  struct sockaddr_in address;
  int opt = 1;
  int addrlen = sizeof(address);

  // Creating socket file descriptor
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    printf("TcpConnexion::iTCPConnecteServeur - Error in socket: %s\n", strerror(errno));
  }

  // Forcefully attaching socket to the port 8080
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                 sizeof(opt))) {
    printf("TcpConnexion::iTCPConnecteServeur - Error in setsockopt: %s\n", strerror(errno));
  }
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;

  if (port < 0) {
    port = PORT;
  }
  address.sin_port = htons(port);

  // Forcefully attaching socket to the port 8080
  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    printf("TcpConnexion::iTCPConnecteServeur - Error in bind: %s\n", strerror(errno));
  }
  if (listen(server_fd, 3) < 0) {
    printf("TcpConnexion::iTCPConnecteServeur - Error in listen: %s\n", strerror(errno));
  }
  if ((socketfd = accept(server_fd, (struct sockaddr *)&address,
                         (socklen_t *)&addrlen)) < 0) {
    printf("TcpConnexion::iTCPConnecteServeur - Error in accept: %s\n", strerror(errno));
  }
  printf("TcpConnexion::iTCPConnecteServeur - Connection established\n");
}

void TcpConnexion::vSend(unsigned char *buffer, int buffSize) {
  // vCSEcriture(true) ;
  // Do not send SIGPIPE in case of broken pipe
  ssize_t sent = send(socketfd, buffer, buffSize, MSG_NOSIGNAL);
  if(sent == -1){
    // Display error if any
    printf("TcpConnexion::vSend - Error in send: %s\n", strerror(errno));
  }
  // vCSEcriture(false) ;
}

bool TcpConnexion::vAssembleTrame() {
  int iNbLus;
  char c;

  bool trameRecue = false;
  // Boucle de lecture du message
  do {
    // Lecture d'un octet sur le socket
    iNbLus = recv(socketfd, &c, 1, 0);
    // Test retour lecture socket
    if (iNbLus > 0) {
      if (m_pVoie->bAjouteCar(c)) {
        trameRecue = m_pVoie->vTraiteEvt(EVT_VOIE_TRAME_HNZ);
        break;
      }
    }
  } while (iNbLus > 0 && loop);

  // Sortie de section critique (pour multithread )
  // vCSEcriture(false) ;

  // m_pcsAcces->Unlock();
  return trameRecue;
}

bool TcpConnexion::is_connected() {
  if (socketfd == -1) {
    return false;
  }
  // logFile("a");
  int error = 0;
  // logFile("b");
  socklen_t len = sizeof(error);
  // logFile("c");
  int retval = getsockopt(socketfd, SOL_SOCKET, SO_ERROR, &error, &len);
  // logFile("d");
  if (retval == -1) {
    printf("TcpConnexion::is_connected - Error in getsockopt: %s\n", strerror(errno));
  }

  return (retval == 0) && (error == 0);
}

void TcpConnexion::stop() {
  loop = false;
  if (socketfd != -1) {
    int res = shutdown(socketfd, SHUT_RDWR);
    if (res == -1) {
      printf("TcpConnexion::stop - Error in shutdown: %s\n", strerror(errno));
    }
    socketfd = -1;
  }
  if (server_fd != -1) {
    int res = close(server_fd);
    if (res == -1) {
      printf("TcpConnexion::stop - Error in close: %s\n", strerror(errno));
    }
    server_fd = -1;
  }
  printf("TcpConnexion::stop - Connection closed\n");
}