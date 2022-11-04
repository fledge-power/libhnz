#include "../inc/TcpConnexion.h"

TcpConnexion::TcpConnexion() {}

TcpConnexion::TcpConnexion(VoieHNZ *voie) { m_pVoie = voie; }

int TcpConnexion::iTCPConnecteClient() {
  iTCPConnecteClient(ADRESSE, PORT);
  printf("Connexion établie\n");
}

int TcpConnexion::iTCPConnecteClient(const char *adresse, int port) {
  struct sockaddr_in serv_addr;
  printf("Adresse de connexion : %s\n", adresse);

  if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("\n Socket creation error \n");
    return -1;
  }

  serv_addr.sin_family = AF_INET;
  if (port == NULL || port < 0) {
    port = PORT;
  }

  if ((!strcmp(adresse, "")) || adresse == NULL) {
    adresse = ADRESSE;
  }

  serv_addr.sin_port = htons(port);

  // Convert IPv4 and IPv6 addresses from text to binary form
  if (inet_pton(AF_INET, adresse, &serv_addr.sin_addr) <= 0) {
    printf("\nInvalid address/ Address not supported \n");
    return -1;
  }

  if (connect(socketfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    printf("\nConnection Failed \n");
    return -1;
  }
  return 0;
}

void TcpConnexion::iTCPConnecteServeur() { iTCPConnecteServeur(PORT); }

void TcpConnexion::iTCPConnecteServeur(int port) {
  int server_fd;
  struct sockaddr_in address;
  int opt = 1;
  int addrlen = sizeof(address);

  // Creating socket file descriptor
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("socket failed");
  }

  // Forcefully attaching socket to the port 8080
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                 sizeof(opt))) {
    perror("setsockopt");
  }
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;

  if (port < 0) {
    port = PORT;
  }
  address.sin_port = htons(port);

  // Forcefully attaching socket to the port 8080
  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("bind failed");
  }
  if (listen(server_fd, 3) < 0) {
    perror("listen");
  }
  if ((socketfd = accept(server_fd, (struct sockaddr *)&address,
                         (socklen_t *)&addrlen)) < 0) {
    perror("accept");
  }
}

void TcpConnexion::vSend(unsigned char *buffer, int buffSize) {
  // vCSEcriture(true) ;
  send(socketfd, buffer, buffSize, NULL);
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
  // logFile("a");
  int error = 0;
  // logFile("b");
  socklen_t len = sizeof(error);
  // logFile("c");
  int retval = getsockopt(socketfd, SOL_SOCKET, SO_ERROR, &error, &len);
  // logFile("d");

  return (retval == 0 && error == 0);
}

int TcpConnexion::stop() {
  loop = false;
  return shutdown(socketfd, SHUT_RDWR);
}