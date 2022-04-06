#ifndef CPORTCP_H
#define CPORTCP_H

#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "VoieHNZ.h"
// port par défaut
#define PORT 1234
#define ADRESSE "127.0.0.1"

// portTCP

class VoieHNZ;

/// Classe utilitaire pour l'entrée en la sortie (automatique) de section critique
class CCSLock
{

public:
    CCSLock(CCriticalSection *pLock)
    {
        m_pLock = pLock;
        m_pLock->Lock();
    }

    ~CCSLock() { m_pLock->Unlock(); }

private:
    CCriticalSection *m_pLock;
};

class TcpConnexion
{
public:
    TcpConnexion();
    TcpConnexion(VoieHNZ *voie);
    void vSend(unsigned char *m_abBuffer, int buffSize);
    void vRecv();
    int iTCPConnecteClient();
    int iTCPConnecteClient(const char *adresse, int port);
    void iTCPConnecteServeur();
    void iTCPConnecteServeur(int port);
    bool vAssembleTrame();
    bool is_connected();
    int socketfd;
    VoieHNZ *m_pVoie;

private:
    CCriticalSection *m_pcsAcces;
};

#endif