#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>


int main(int argc, char* argv[])
{   
    /* Defining TCP client */
    printf("\n*** Im a TCP client ***\n");
    struct sockaddr_in my_addr;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    int reuse= 1;

    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    memset((char*)&my_addr, 0, sizeof(my_addr));
    inet_aton(argv[1], &my_addr.sin_addr);
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(atoi(argv[2]));;


    printf("socket : %d \n", sock);
    if (sock == 0){
        printf("Return value is : 0");
        return 0;
    }
    if (sock <= 0){
        printf("Return value is : %d", sock);
        return 1;
    }
    
    int connexion;
    connexion = connect(sock, (struct sockaddr*)&my_addr, sizeof(my_addr));

    printf("I m connexion value : %d \n", connexion);

    char buffer[20];
    read(sock, buffer, 20*sizeof(char));
    printf("reading : %s \n", buffer);

    char *sos = "Hi!";
    write(sock, sos, strlen(buffer));
}