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

struct segment {
    size_t size;
    unsigned short number;
    char data[SIZE];
};

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
    char *buffer_udp = "SOS";
    sendto(sock_udp, buffer_udp, strlen(buffer_udp)+1, 0, (struct sockaddr*)&my_addr_udp, sizeof(my_addr_udp));
    char *buffer_ud = "SYN";
    sendto(sock_udp, buffer_ud, strlen(buffer_ud)+1, 0, (struct sockaddr*)&my_addr_udp, sizeof(my_addr_udp));


    // Trying to receive SYN_ACK
    char buffer_udp_syn_ack[MSG_LEN];
    unsigned int udp_size = sizeof(my_addr_udp);
    do {recvfrom(sock_udp, &buffer_udp_syn_ack, MSG_LEN, 0, (struct sockaddr*)&my_addr_udp, &udp_size);
        printf("Waiting for SYN_ACK... Reading : %s \n", buffer_udp_syn_ack);
    } while(strcmp(buffer_udp_syn_ack, "SYN_ACK") !=0 );
 
    // Sending ACK
    char *buffer_udp_ack = "ACK";
    sendto(sock_udp, buffer_udp_ack, strlen(buffer_udp_ack)+1, 0, (struct sockaddr*)&my_addr_udp, sizeof(my_addr_udp));

    // Changing port for data message
    my_addr_udp.sin_port = htons(atoi(argv[3]));
    printf("\n** Switching to port : %d \n", ntohs(my_addr_udp.sin_port));

    // Sending data message
    char *buffer_udp_msg = "Hello, I m a useful message";
    sendto(sock_udp, buffer_udp_msg, strlen(buffer_udp_msg)+1, 0, (struct sockaddr*)&my_addr_udp, sizeof(my_addr_udp));
    printf("Useful message sent \n");


    // Starting to receive a wonderful jpeg(obviously a cat)
    FILE* fp = fopen("cat.jpeg", "w");
    char buffer_file_transmit[SIZE];
    unsigned int my_addr_len = sizeof(my_addr_udp);
    struct segment segm;
    segm.size = 0;
    while(1)
    {   
        // Receiving parts of the file
        recvfrom(sock_udp, (char*)&segm, sizeof(struct segment), 0, (struct sockaddr*)&my_addr_udp, &my_addr_len);
        
        // Always checking if it's the end of the file
        if (strncmp((char*)&segm, "I_AM_END_OF_FILE", 17) == 0)
        {
            printf("EOF ! \n");
            break;
        }

        char received_msg[11];
        sprintf(received_msg, "ACK_%d", segm.number);
        printf("%s, %d \n", received_msg, segm.size);
        sendto(sock_udp, received_msg, strlen(received_msg)+1, 0, (struct sockaddr*)&my_addr_udp, sizeof(my_addr_udp));

        // If not EOF, printing the size of received packet & wrtiting it in the destinaion file & sending ACK
        //printf("recevied package : ACK_%d n°%d \n", n, compteur);
        fwrite(segm.data, 1, segm.size, fp);
        memset(buffer_file_transmit, 0, SIZE);
    }
    fclose(fp);        
}