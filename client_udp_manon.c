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

#define MSG_LEN 60 //for UDP buffer
#define SIZE 1024

int main(int argc, char* argv[])
{   
    /* Defining UDP client */
    printf("\n*** Im a UDP client ***\n");
    struct sockaddr_in my_addr_udp;
    int sock_udp = socket(AF_INET, SOCK_DGRAM, 0);
    int reuse_udp = 1;

    setsockopt(sock_udp, SOL_SOCKET, SO_REUSEADDR, &reuse_udp, sizeof(reuse_udp));
    memset((char*)&my_addr_udp, 0, sizeof(my_addr_udp));
    inet_aton(argv[1], &my_addr_udp.sin_addr);
    my_addr_udp.sin_family = AF_INET;
    my_addr_udp.sin_port = htons(atoi(argv[2]));


    // Sending SYN
    printf("port : %d \n", ntohs(my_addr_udp.sin_port));
    char *buffer_udp = "SYN";
    sendto(sock_udp, buffer_udp, strlen(buffer_udp)+1, 0, (struct sockaddr*)&my_addr_udp, sizeof(my_addr_udp));

    // Trying to receive SYN_ACK
    char buffer_udp_syn_ack[MSG_LEN];
    unsigned int udp_size = sizeof(my_addr_udp);
    recvfrom(sock_udp, &buffer_udp_syn_ack, MSG_LEN, 0, (struct sockaddr*)&my_addr_udp, &udp_size);
    printf("reading : %s \n", buffer_udp_syn_ack);

    int result;
    result = strcmp(buffer_udp_syn_ack, "SYN_ACK");
    // if SYN_ACK is received, sending ACK
    if (result == 0){
        char *buffer_udp_ack = "ACK";
        sendto(sock_udp, buffer_udp_ack, strlen(buffer_udp_ack)+1, 0, (struct sockaddr*)&my_addr_udp, sizeof(my_addr_udp));

        // Changing port for data message
        my_addr_udp.sin_port = htons(atoi(argv[3]));
        printf("port : %d \n", ntohs(my_addr_udp.sin_port));
        // Sending data message
        char *buffer_udp_msg = "Hello, I m a useful message";
        sendto(sock_udp, buffer_udp_msg, strlen(buffer_udp_msg)+1, 0, (struct sockaddr*)&my_addr_udp, sizeof(my_addr_udp));
        printf("Useful message sent \n");



        // Sending the file
        int n;
        char buffer[SIZE];

        FILE* fp = fopen("index.jpeg", "r");
        int size = fseek(fp, 0L, SEEK_END);
        long int res = ftell(fp);
        printf("Size file %d \n", res);

        //char *size_of_file = res;
        sendto(sock_udp, res, res, 0, (struct sockaddr*)&my_addr_udp, sizeof(my_addr_udp));

        while (!feof(fp))
        {   
            int n = fread(buffer, 1, SIZE, fp);
            printf("[SENDING] Data: %s \n", buffer);

            n = sendto(sock_udp, buffer, n, 0, (struct sockaddr*)&my_addr_udp, sizeof(my_addr_udp));
            if (n == -1)
            {
            perror("[ERROR] sending data to the server.");
            exit(1);
            }
            bzero(buffer, SIZE);
        }

        fclose(fp);
        printf("File sent \n");
    }
    else return 1;

}