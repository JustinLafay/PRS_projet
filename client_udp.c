#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define RCVSIZE 1024
#define PORT 5001

int main (int argc, char *argv[]) {
  int port = 5001;
  int ip = INADDR_LOOPBACK;
  printf("%s\n",argv[0]);
  if(argc>=2){
    printf("nb arg :  %d\n valeur : %s\n",argc, argv[1]);
    ip = atoi(argv[1]);
    if (argc == 3){
      port = atoi(argv[2]);
    }
  }
  printf("ip : %d\n",ip);
  printf("port : %d\n",port);
  struct sockaddr_in adresse;
  socklen_t alen= sizeof(adresse);
  
  int valid = 1;
  char msg[RCVSIZE];
  char blanmsg[RCVSIZE];

  //create socket
  int server_desc = socket(AF_INET, SOCK_DGRAM, 0);
  int server_donnee = socket(AF_INET, SOCK_DGRAM, 0);

  if (server_desc < 0) {
    perror("cannot create socket\n");
    return -1;
  }

  setsockopt(server_desc, SOL_SOCKET, SO_REUSEADDR, &valid, sizeof(int));

  adresse.sin_family= AF_INET;
  adresse.sin_port= htons(port);
  adresse.sin_addr.s_addr= htonl(ip);

  printf("Entering infinite while...\n\n\n");

  int cont= 1;
  while (cont) {
    // fgets(msg, RCVSIZE, stdin);
    char* data = "SYN";
    char delim[] = " ";

    //send
    int server_message = sendto(server_desc, data, strlen(data)+1, MSG_CONFIRM, (struct sockaddr *) &adresse, sizeof(adresse));
    printf("Send :%s\n", data);
    int client_receive = recvfrom(server_desc, (char *)blanmsg, RCVSIZE, MSG_WAITALL, ( struct sockaddr *) &adresse, &alen);
    printf("Receive :%s\n", blanmsg);
    char *str = strtok(blanmsg, delim);
    if (strcmp("SYN-ACK",str)==0) {
      printf("NEW PORT :%s\n", &blanmsg[8]);
      data = "ACK";
      server_message = sendto(server_desc, data, strlen(data)+1, MSG_CONFIRM, (struct sockaddr *) &adresse, sizeof(adresse));
      printf("Send :%s\n", data);
      port = atoi(&blanmsg[8]);
      adresse.sin_port= htons(port);

      data = " (0_0) ";
      server_message = sendto(server_donnee, data, strlen(data)+1, MSG_CONFIRM, (struct sockaddr *) &adresse, sizeof(adresse));
      printf("Send :%s\n", data);
      cont =0;
    }
    memset(blanmsg,0,RCVSIZE);
  }
//closesocket
close(server_desc);
return 0;
}