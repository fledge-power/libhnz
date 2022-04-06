#include "../inc/TcpConnexion.h"
#include "../inc/VoieHNZ.h"
#include "../inc/hnz_server.h"
#include <thread>
#include <stdio.h>
#include <stdlib.h>

int main(void) {


    HNZServer* server = new HNZServer();
    server->start(6001);

    MSG_TRAME *myFr = new MSG_TRAME;

    unsigned char message[4];
    message[0] = 0x17;
    message[1] = 0x0F;
    message[2] = 0x29;
    message[3] = 0x2F;

    printf("Envoi SARM\n");
    server->addMsgToFr(myFr,message,4);
    server->sendFr(myFr);

    sleep(1);
    MSG_TRAME *myFr1 = new MSG_TRAME;
    printf("Envoi trame bulle\n");
    unsigned char message1[6];
    message1[0] = 0x15;
    message1[1] = 0x08;
    message1[2] = 0x13;
    message1[3] = 0x04;
    message1[4] = 0x37;
    message1[5] = 0x6E;
    server->addMsgToFr(myFr1,message1,6);
    server->sendFr(myFr1);

    sleep(1);
    MSG_TRAME *myFr4 = new MSG_TRAME;
    printf("Envoi module 10min\n");
    unsigned char message4[6];
    message4[0] = 0x05;  // 05 4C 0F 48 D8 7C
    message4[1] = 0x4C;
    message4[2] = 0x0F;
    message4[3] = 0x48;
    message4[4] = 0xD8;
    message4[5] = 0x7C;
    server->addMsgToFr(myFr4,message4,6);
    server->sendFr(myFr4);

    sleep(1);
    MSG_TRAME *myFr3 = new MSG_TRAME;
    printf("Envoi trame TSCE\n");
    unsigned char message3[9];
    message3[0] = 0x05;
    message3[1] = 0x84;
    message3[2] = 0x0B;
    message3[3] = 0x33;
    message3[4] = 0x28;
    message3[5] = 0x36;
    message3[6] = 0xF2;
    message3[7] = 0x18;
    message3[8] = 0xB4;
    server->addMsgToFr(myFr3,message3,9);
    server->sendFr(myFr3);


    // // Envoi de 15 02 02 00 00 00 00 00 0B 02 E9 D7 45 02 04 00 F8 00 00 28 DE
    // sleep(1);
    // MSG_TRAME *myFr2 = new MSG_TRAME;
    // printf("Envoi trame info avec 3 messages (TM4, TSCE, TM4) \n");
    // unsigned char message2[21];
    // message2[0] = 0x15;
    // message2[1] = 0x02;
    // message2[2] = 0x02;
    // message2[3] = 0x00;
    // message2[4] = 0x00;
    // message2[5] = 0x00;
    // message2[6] = 0x00;
    // message2[7] = 0x00;
    // message2[8] = 0x0B;
    // message2[9] = 0x02;
    // message2[10] = 0x00;
    // message2[11] = 0xD7;
    // message2[12] = 0x45;
    // message2[13] = 0x02;
    // message2[14] = 0x04;
    // message2[15] = 0x00;
    // message2[16] = 0xF8;
    // message2[17] = 0x00;
    // message2[18] = 0x00;
    // message2[19] = 0x28;
    // message2[20] = 0xDE;
    // server->addMsgToFr(myFr2,message2,21);
    // server->sendFr(myFr2);

    // memcpy(message+2,"\x0B",1);


    // for (int i = 0; i<5; i++){
    //     server->addMsgToFr(myFr,message);
    //     server->sendFr(myFr);
    //     sleep(1);
    // }

    while(1){
        unsigned char* data = nullptr;
        while(1) {
            data = (server->receiveData());
            if (data != nullptr){
                break;
            }
        }

        unsigned char c = data[1];
        int len = 0;
        switch (c){
            case UA:
                printf("UA reçu");
                len = 0;
                break;
            default:
                len = strlen((char *) data);
                break;
        }

        if (len != 0){
            unsigned char content[len+1];
            memcpy(content,data+3, len);

            printf("Message reçu : %02X\n", (unsigned int*)(content));
        }

    }

    server->stop();
}

