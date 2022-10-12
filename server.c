#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define RCVSIZE 1024

int main (int argc, char *argv[]) {

  struct sockaddr_in adresse, client;
  int port= 5001;
  int valid= 1;
  socklen_t alen= sizeof(client);
  char buffer[RCVSIZE];

  //create socket
  int server_desc = socket(AF_INET, SOCK_STREAM, 0);

  if (server_desc < 0) {
    perror("Cannot create socket\n");
    return -1;
  }

  setsockopt(server_desc, SOL_SOCKET, SO_REUSEADDR, &valid, sizeof(int));

  adresse.sin_family= AF_INET;
  adresse.sin_port= htons(port);
  adresse.sin_addr.s_addr= htonl(INADDR_ANY);

  //bind (initialize) socket
  if (bind(server_desc, (struct sockaddr*) &adresse, sizeof(adresse)) == -1) {
    perror("Bind failed\n");
    close(server_desc);
    return -1;
  }


  //listen to incoming clients
  if (listen(server_desc, 0) < 0) {
    printf("Listen failed\n");
    return -1;
  }

  printf("Listen done\n");

  while (1) {

    printf("Accepting\n");
    //accept
    int client_desc = accept(server_desc, (struct sockaddr*)&client, &alen);

    printf("Value of accept is:%d\n", client_desc);
    //recv
    int msgSize = read(client_desc,buffer,RCVSIZE);

    while (msgSize > 0) {
      //send
      write(client_desc,buffer,msgSize);
      printf("%s",buffer);
      memset(buffer,0,RCVSIZE);
      //recv bis
      msgSize = read(client_desc,buffer,RCVSIZE);
    }

    close(client_desc);

  }
//closesocket
close(server_desc);
return 0;
}
