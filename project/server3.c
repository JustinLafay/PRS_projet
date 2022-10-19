#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define MSG_LEN 60          // for UDP buffer
#define MSG_LEN_USEFUL 700  // for UDP buffer

#define SIZE 1024

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

int main(int argc, char *argv[]) {
  int port = 1500;
  /** Server running ... **/
  while (1) {
    printf("Entering server...");
    /** Defining UDP CONTROL socket **/
    struct sockaddr_in my_addr_udp;
    int reuse_udp = 1;
    int sock_udp = create_sock(&my_addr_udp, atoi(argv[2]));

    struct sockaddr_in my_addr_udp_client;
    fd_set im_socket;
    FD_SET(sock_udp, &im_socket);
    printf("\n ======= \n");
    select(5, &im_socket, NULL, NULL, NULL);

    /** If socket is UDP **/
    if (FD_ISSET(sock_udp, &im_socket)) {
      printf("Receving UDP message \n");
      printf("** port : %d \n", ntohs(my_addr_udp.sin_port));

      // waiting to receive SYN
      char buffer_udp[MSG_LEN];
      unsigned int udp_size = sizeof(my_addr_udp);
      do {
        recvfrom(sock_udp, &buffer_udp, MSG_LEN, 0, (struct sockaddr *)&my_addr_udp, &udp_size);
        printf("Waiting for SYN... Reading : %s \n", buffer_udp);
      } while (strcmp(buffer_udp, "SYN") != 0);

      port = port + 1;
      printf("portincr : %d\n", port);

      /** Defining UDP DATA socket **/
      struct sockaddr_in data_msg_udp;
      int sock_udp_data = create_sock(&data_msg_udp, port);

      // Sending SYN_ACK
      char buffer_udp_syn_ack[11];
      sprintf(buffer_udp_syn_ack, "SYN-ACK%d", ntohs(data_msg_udp.sin_port));
      printf("sending SYN_ACKXXXX ...\n");
      sendto(sock_udp, buffer_udp_syn_ack, strlen(buffer_udp_syn_ack) + 1, 0, (struct sockaddr *)&my_addr_udp, sizeof(my_addr_udp));

      // Receiving ACK
      char buffer_udp_ack[MSG_LEN];
      unsigned int udp_size_ack = sizeof(my_addr_udp);
      do {
        recvfrom(sock_udp, &buffer_udp_ack, MSG_LEN, 0, (struct sockaddr *)&my_addr_udp, &udp_size_ack);
        printf("Waiting for ACK... Reading : %s \n", buffer_udp_ack);
      } while (strcmp(buffer_udp_ack, "ACK") != 0);

      printf("ACK exchanges completed succesfully \n");

      // CREATION DE FORK DONNEE
      int x = fork();

      // SEPARATION PARENT/ENFANT
      if (x != 0) {
        close(sock_udp_data);
        continue;
      }

      // FERMER SOCKET ACCUEIL POUR ENFANT
      close(sock_udp);

      printf("\n** Switching to port : %d \n", port);
      // Receiving data message 'hello, I'm a useful message'
      char buffer_udp_msg[MSG_LEN_USEFUL];
      // unsigned int udp_size_ack = sizeof(data_msg_udp);
      recvfrom(sock_udp_data, &buffer_udp_msg, MSG_LEN_USEFUL, 0, (struct sockaddr *)&data_msg_udp, &udp_size_ack);
      printf("reading : %s \n", buffer_udp_msg);

      // Sending the jpeg file
      int n;
      // char buffer[SIZE];
      FILE *fp = fopen("image.jpeg", "r");
      int size = fseek(fp, 0L, SEEK_END);
      long int res = ftell(fp);
      printf("Size file %d \n", res);

      // Going at the begening of the file to transmmit the begining
      fseek(fp, 0L, SEEK_SET);
      char buffer_file[SIZE];
      // memset(buffer_file, 0, sizeof(buffer_file));
      int sequence_number = 1;

      struct timeval tv;
      tv.tv_sec = 0;
      tv.tv_usec = 100000;

      int chunk_file;
      char seq_num[1032];

      // While not end of file
      while (!feof(fp)) {
        // Reading the file & sending a part
        chunk_file = fread(buffer_file, 1, SIZE, fp);
        sprintf(seq_num, "%06d", sequence_number);
        memcpy(seq_num + 6, buffer_file, chunk_file);
        sendto(sock_udp_data, seq_num, SIZE, 0, (struct sockaddr *)&my_addr_udp, sizeof(my_addr_udp));

        n = recvfrom(sock_udp_data, buffer_file, SIZE, 0, (struct sockaddr *)&data_msg_udp, &udp_size_ack);
        printf("received : %s \n", buffer_file);

        if (setsockopt(sock_udp_data, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
          perror("Error");
        }

        // all good
        if (n > 0) {
          printf("received : %s \n", buffer_file);
        }

        // if error in receiving (< 0)
        else {
          do {
            printf("Error when receiving (%d), sending again %d\n", n, sequence_number);
            sendto(sock_udp_data, seq_num, SIZE, 0, (struct sockaddr *)&my_addr_udp, sizeof(my_addr_udp));
            n = recvfrom(sock_udp_data, buffer_file, SIZE, 0, (struct sockaddr *)&data_msg_udp, &udp_size_ack);
            printf("received BIS : %s \n", buffer_file);
          } while (n < 0);
        }

        // FD_SET(socketListener, &readfds);

        // Making sure its the correct ACK
        if (atoi(&buffer_file[3]) != sequence_number)
          printf("expected received msg : %d \n", sequence_number);

        if (n == -1) {
          perror("[ERROR] sending data to the server.\n");
          exit(1);
        }
        memset(buffer_file, 0, sizeof(buffer_file));
        sequence_number += 1;
      }
      fclose(fp);
      // Sending it's the end of the file to the server
      char *file_ended = "FIN";
      sendto(sock_udp_data, file_ended, strlen(file_ended) + 1, 0, (struct sockaddr *)&my_addr_udp, sizeof(my_addr_udp));
      printf("**File sent : EOF**\n");

      // FERMER SOCKET DONNEE
      close(sock_udp_data);

      // FIN DE LA FORK CHILD
      exit(x);
    }
  }
}
