#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define RCVSIZE 1024
#define PORT 5001

int port = 5002;
char buffer[RCVSIZE];

// FONCTION CREATION DE SOCKET

int create_sock(struct sockaddr_in *addr_ptr, int port) {
  int valid = 1;
  int sock = socket(AF_INET, SOCK_DGRAM, 0);

  if (sock < 0) {
    perror("Cannot create socket\n");
    return -1;
  }

  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &valid, sizeof(int));

  addr_ptr->sin_family = AF_INET;
  addr_ptr->sin_port = htons(port);
  addr_ptr->sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(sock, (struct sockaddr *)addr_ptr, sizeof(struct sockaddr_in)) == -1) {
    perror("Bind failed\n");
    close(sock);
    return -1;
  }

  return sock;
}

// MAIN

int main(int argc, char *argv[]) {
  printf("Entering infinite while...\n\n\n");

  while (1) {
    // CREATION SOCKET ACCUEIL CLIENT
    struct sockaddr_in adresse;
    socklen_t alen = sizeof(adresse);
    int server_desc = create_sock(&adresse, PORT);

    // ATTENTE SYN
    int server_receive = recvfrom(server_desc, (char *)buffer, RCVSIZE, MSG_WAITALL, (struct sockaddr *)&adresse, &alen);
    printf("Receive : %s\n", buffer);

    if (strcmp("SYN", buffer) == 0) {
      // INCREMENTATION DU PORT CLIENT
      port = port + 1;

      // CREATION SOCKET TRANSFERT DONNEE CLIENT
      struct sockaddr_in client;
      socklen_t clen = sizeof(client);
      int server_donnee = create_sock(&client, port);

      // ECRITURE PORT SYN-ACK PIGGYBACKING
      char data[14] = "SYN-ACK ";
      char port_str[5];
      sprintf(port_str, "%d", port);
      strcat(data, port_str);

      // ENVOI SYN-ACK ET PORT
      int client_message = sendto(server_desc, data, strlen(data) + 1, MSG_CONFIRM, (struct sockaddr *)&adresse, sizeof(adresse));
      printf("Send : %s\n", data);

      // ATTENTE ACK
      server_receive = recvfrom(server_desc, (char *)buffer, RCVSIZE, MSG_WAITALL, (struct sockaddr *)&adresse, &alen);

      if (strcmp("ACK", buffer) == 0) {
        printf("Change port to : %d\n", port);

        // CREATION DE FORK DONNEE
        int x = fork();

        // SEPARATION PARENT/ENFANT
        if (x != 0) {
          close(server_donnee);
          continue;
        }

        // FERMER SOCKET ACCUEIL POUR ENFANT
        close(server_desc);

        // RECEPTION DE DONNEE
        server_receive = recvfrom(server_donnee, (char *)buffer, RCVSIZE, MSG_WAITALL, (struct sockaddr *)&client, &alen);

        // IMPRESSION DE DONNEE
        printf("Data on new port : %s\n", buffer);
        memset(buffer, 0, RCVSIZE);

        // SIMUL CALCUL
        sleep(3);

        printf("Closing child fork...%d\n", x);

        // FERMER SOCKET DONNEE
        close(server_donnee);

        // FIN DE LA FORK CHILD
        exit(0);
      }
    }
  }
}