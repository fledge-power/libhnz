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
    printf("TcpConnexion::iTCPConnecteClient - Error in setsockopt SO_RCVTIMEO: %s\n", strerror(errno));
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
  m_is_connected = true;
  loop = true;
  printf("TcpConnexion::iTCPConnecteClient - Connection established (socket %d)\n", socketfd);
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
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
    printf("TcpConnexion::iTCPConnecteServeur - Error in setsockopt SO_REUSEADDR | SO_REUSEPORT: %s\n", strerror(errno));
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
  // Set timeout for recv() calls to avoid busy loop while waiting for messages
  long long int recvTimeoutUs = RECV_TIMEOUT;
  struct timeval tv;
  tv.tv_sec = static_cast<time_t>(recvTimeoutUs / 1000000);
  tv.tv_usec = static_cast<long int>(recvTimeoutUs - (tv.tv_sec * 1000000));
  if (setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv)) {
    printf("TcpConnexion::iTCPConnecteServeur - Error in setsockopt SO_RCVTIMEO: %s\n", strerror(errno));
  }

  m_is_connected = true;
  loop = true;
  printf("TcpConnexion::iTCPConnecteServeur - Connection established (socket %d, listen %d)\n", socketfd, server_fd);
}

void TcpConnexion::vSend(unsigned char *buffer, int buffSize) {
  // vCSEcriture(true) ;
  int iNbSent = 0;
  int iNbTotalSent = 0;
  // Message send loop
  do {
    // printf("TcpConnexion::vSend - socket %d - Try sending data...\n", socketfd);
    // Do not send SIGPIPE in case of broken pipe (MSG_NOSIGNAL)
    int iNbSent = send(socketfd, buffer + iNbTotalSent, buffSize - iNbTotalSent, MSG_NOSIGNAL);
    if(iNbSent == -1){
      // Display error if any
      printf("TcpConnexion::vSend - socket %d - Error in send: %s\n", socketfd, strerror(errno));
      m_is_connected = false;
    }
    else {
      iNbTotalSent += iNbSent;
      // if (iNbTotalSent < buffSize) {
      //   printf("TcpConnexion::vSend - socket %d - Sent bytes %d/%d\n", socketfd, iNbSent, buffSize);
      // }
      // else {
      //   printf("TcpConnexion::vSend - socket %d - Sent full frame\n", socketfd);
      // }
    }
  } while ((iNbTotalSent < buffSize) && (iNbSent > 0) && loop);
  // vCSEcriture(false) ;
}

bool TcpConnexion::vAssembleTrame() {
  int iNbLus;
  char c;

  bool trameRecue = false;
  // Message reception loop
  do {
    // Lecture d'un octet sur le socket
    iNbLus = recv(socketfd, &c, 1, 0);
    // Test retour lecture socket
    if (iNbLus > 0) {
      // printf("TcpConnexion::vAssembleTrame - socket %d - Received byte: %02X\n", socketfd, static_cast<unsigned int>(c) & 0xFF);
      if (m_pVoie->bAjouteCar(c)) {
        trameRecue = m_pVoie->vTraiteEvt(EVT_VOIE_TRAME_HNZ);
        // if (trameRecue) {
        //   printf("TcpConnexion::vAssembleTrame - socket %d - Received full frame\n", socketfd);
        // }
        break;
      }
    }
    else {
      if (iNbLus == 0) {
        // Receiving 0 bytes indicates that the connection was gracefully closed by the other side
        printf("TcpConnexion::vAssembleTrame - No bytes received: connection closed\n");
        m_is_connected = false;
      }
      else {
        // Display error if any (ignore EAGAIN and EWOULDBLOCK as they are expected in case of recv timeout)
        if ((errno != EAGAIN) && (errno != EWOULDBLOCK)) {
          printf("TcpConnexion::vAssembleTrame - socket %d - Error in recv: %s\n", socketfd, strerror(errno));
          m_is_connected = false;
        }
      }
    }
  } while ((iNbLus > 0) && loop);

  // Sortie de section critique (pour multithread )
  // vCSEcriture(false) ;

  // m_pcsAcces->Unlock();
  return trameRecue;
}

bool TcpConnexion::is_connected() {
  return m_is_connected;
}

// Alternative method to check if socket is still connected
bool TcpConnexion::check_connection() {
  if (socketfd == -1) {
    return false;
  }

  int error = 0;
  socklen_t len = sizeof(error);
  int retval = getsockopt(socketfd, SOL_SOCKET, SO_ERROR, &error, &len);
  if (retval == -1) {
    printf("TcpConnexion::check_connection - socket %d - Error in getsockopt: %s\n", socketfd, strerror(errno));
  }

  return (retval == 0) && (error == 0);
}

void TcpConnexion::stop() {
  loop = false;
  if (socketfd != -1) {
    int res = shutdown(socketfd, SHUT_RDWR);
    if (res == -1) {
      printf("TcpConnexion::stop - socket %d - Error in shutdown: %s\n", socketfd, strerror(errno));
    }
    // else {
    //   printf("TcpConnexion::stop - socket %d - Shutdown socket\n", socketfd);
    // }
    res = close(socketfd);
    if (res == -1) {
      printf("TcpConnexion::stop - socket %d - Error in close: %s\n", socketfd, strerror(errno));
    }
    // else {
    //   printf("TcpConnexion::stop - socket %d - Closed socket\n", socketfd);
    // }
    socketfd = -1;
  }
  if (server_fd != -1) {
    int res = close(server_fd);
    if (res == -1) {
      printf("TcpConnexion::stop - socket %d - Error in close listen: %s\n", server_fd, strerror(errno));
    }
    // else {
    //   printf("TcpConnexion::stop - socket %d - Closed listen socket\n", server_fd);
    // }
    server_fd = -1;
  }
  m_is_connected = false;
  printf("TcpConnexion::stop - Connection closed\n");
}