//
// Created by Lucas Barret on 06/04/2021.
//

#include "../inc/TcpConnexion.h"
#include "../inc/hnz_client.h"

int main(void) {
  HNZClient* client = new HNZClient();
  client->connect_Server("127.0.0.1", 6001);
  // client->connect_Server("192.168.176.134",6001);
  while (1) {
    unsigned char* data = nullptr;
    while (1) {
      data = (client->receiveData());
      if (data != nullptr) {
        break;
      }
    }

    unsigned char c = data[2];
    int len = 0;

    MSG_TRAME* myFr = new MSG_TRAME;
    unsigned char message[12] = "123456";
    memcpy(message + 2, "\x63", 1);
    memcpy(message + 3, "ABCDEFGH", 8);

    switch (c) {
      case TM4_CODE:
        printf("TM4_CODE reçu\n");
        len = 6;
        break;
      case TSCE_CODE:
        printf("TSCE_CODE reçu\n");
        len = 5;
        break;
      case SARM_CODE:
        printf("SARM_CODE reçu\n");
        printf("Envoi UA\n");
        client->addMsgToFr(myFr, message);
        client->sendFr(myFr);
        len = 0;
        break;
      default:
        len = strlen((char*)data);
        break;
    }

    if (len != 0) {
      unsigned char content[len + 1];
      memcpy(content, data + 3, len);

      printf("Message reçu : %s\n", content);
    }
  }
}