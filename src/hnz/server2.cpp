#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <thread>

#include "../inc/TcpConnexion.h"
#include "../inc/VoieHNZ.h"
#include "../inc/hnz_server.h"

using namespace std;
using namespace std::chrono;

HNZServer *server = new HNZServer();
atomic<bool> is_running;

void receiving_loop() {
  while (is_running) {
    unsigned char *data = nullptr;
    while (1) {
      data = (server->receiveData());
      if (data != nullptr) {
        break;
      }
    }

    unsigned char c = data[1];
    int len = 0;
    switch (c) {
      case UA_CODE:
        printf("UA reçu\n");
        len = 0;
        break;
      case SARM_CODE:
        printf("SARM reçu\n");
        len = 0;
        break;
      default:
        len = strlen((char *)data);
        break;
    }

    if (len != 0) {
      if (data[1] & 0x1) {
        cout << "> RR reçu : ";
        cout << hex << (int)((data[1] >> 5) & 0x7);
        cout << endl;
      } else {
        cout << "> Message reçu : ";

        for (size_t i = 0; i < len; i++) {
          cout << hex << (int)data[i] << " ";
        }

        cout << endl;

        // Sending RR
        // TODO : add repetition if frame is repeted
        int f = (data[1] >> 4) & 0x1;
        int nr = ((data[1] >> 1) & 0x7) + 1;
        printf("< Envoi RR %d %s\n", nr, (f ? "rep" : ""));

        unsigned char message[1]{(nr << 5) + 1 + 0x10 * f};
        server->createAndSendFr(data[0], message, sizeof(message));
      }
    }
    // this_thread::sleep_for(milliseconds(1000));
  }
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Port is required in args (6001 for example)\n");
    return -1;
  }
  server->start(atoi(argv[1]));

  unsigned char message[1];
  message[0] = 0x0F;
  server->createAndSendFr(0x05, message, sizeof(message));
  printf("Envoi SARM\n");

  bool ua_ok = false;
  bool sarm_ok = false;

  while (1) {
    unsigned char *data = server->receiveData();
    unsigned char c = data[1];
    switch (c) {
      case UA_CODE:
        printf("UA reçu\n");
        ua_ok = true;
        break;
      case SARM_CODE:
        printf("SARM reçu\n");
        unsigned char message[1];
        message[0] = 0x63;
        printf("Envoi UA\n");
        server->createAndSendFr(0x07, message, sizeof(message));
        sarm_ok = true;
        break;
      default:
        printf("Neither UA onr SARM\n");
        break;
    }
    if (ua_ok && sarm_ok) break;
    // this_thread::sleep_for(milliseconds(100));
  }

  // Connection established
  is_running = true;
  thread *receiving_thread = new thread(receiving_loop);

  // Send information message to test

  /*   this_thread::sleep_for(milliseconds(1000));
    printf("Envoi trame bulle\n");
    unsigned char message1[3]{0x00, 0x13, 0x04};
    server->createAndSendFr(0x05, message1, sizeof(message1));

    printf("Envoi module 10min\n");
    unsigned char message4[4]{0x02, 0x4C, 0x0F, 0x48};
    server->createAndSendFr(0x05, message4, sizeof(message4));

    this_thread::sleep_for(milliseconds(5000));
    printf("Envoi trame TSCE\n");
    unsigned char message3[6]{0x04, 0x0B, 0x33, 0x28, 0x36, 0xF2};
    server->createAndSendFr(0x05, message3, sizeof(message3)); */

  // // Envoi de 15 02 02 00 00 00 00 00 0B 02 E9 D7 45 02 04 00 F8 00 00 28
  // DE this_thread::sleep_for(milliseconds(1000)); MSG_TRAME
  // *myFr2 = new MSG_TRAME; printf("Envoi trame info avec 3 messages (TM4,
  // TSCE, TM4) \n"); unsigned char message2[21]; message2[0] = 0x15;
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

  this_thread::sleep_for(milliseconds(1200000));

  // Stop thread
  is_running = false;
  receiving_thread->join();

  server->stop();
}
