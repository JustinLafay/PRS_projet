#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define RCVSIZE 1024

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
  
  int valid = 1;
  char msg[RCVSIZE];
  char blanmsg[RCVSIZE];

  //create socket
  int server_desc = socket(AF_INET, SOCK_STREAM, 0);

  if (server_desc < 0) {
    perror("cannot create socket\n");
    return -1;
  }

  setsockopt(server_desc, SOL_SOCKET, SO_REUSEADDR, &valid, sizeof(int));

  adresse.sin_family= AF_INET;
  adresse.sin_port= htons(port);
  adresse.sin_addr.s_addr= htonl(ip);

  // connect
  int rc = connect(server_desc, (struct sockaddr*)&adresse, sizeof(adresse));
  printf("value of connect is: %d\n", rc);

  if (rc < 0) {
    perror("connect failed\n");
    return -1;
  }

  int cont= 1;
  while (cont) {
    fgets(msg, RCVSIZE, stdin);
    //send
    int sss= write(server_desc,msg,strlen(msg));
    printf("the value of sent is:%d\n", sss);
    //recv
    read(server_desc, blanmsg, RCVSIZE);
    printf("%s",blanmsg);
    memset(blanmsg,0,RCVSIZE);
    if (strcmp(msg,"stop\n") == 0) {
      cont= 0; 
    }
  }
//closesocket
close(server_desc);
return 0;
}