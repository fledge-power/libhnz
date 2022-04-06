//
// Created by Lucas Barret on 06/04/2021.
//

#include "../inc/TcpConnexion.h"
#include "../inc/VoieHNZ.h"
#include "../inc/hnz_server.h"
#include <thread>
#include <stdio.h>
#include <stdlib.h>

int main(void) {

    /*VoieHNZ* voieHnz = new VoieHNZ();

    TcpConnexion slave (voieHnz);
    printf("Truc11111\n");
    voieHnz->setSlave(&slave);
    printf("Aprés set slave\n");
    slave.iTCPConnecteServeur(1234);
    printf("Aprés connection server\n");
    const char* message = "Truc";
    MSG_TRAME *mymsg = new MSG_TRAME;
    memcpy(mymsg->aubTrame,message,5);
    printf("Truc22222\n");
    mymsg->usLgBuffer = 5;

    for (int i = 0; i < 5; i++) {
        slave.m_pVoie->vEnvoiTrameVersHnz(mymsg);
        printf("Truccccc %s\n", mymsg->aubTrame);
    }*/

    HNZServer* server = new HNZServer();
    server->start(1234);

    const char* message = "Tr";

    for (int i = 0; i<5; i++){
        server->createAndSendFr(message);
        sleep(1);
    }


    server->stop();
}