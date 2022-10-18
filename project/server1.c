#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>

#define MSG_LEN 60 //for UDP buffer
#define MSG_LEN_USEFUL 700 //for UDP buffer

#define SIZE 1024


int main(int argc, char* argv[])
{   
    /** Defining UDP CONTROL socket **/
    struct sockaddr_in my_addr_udp;
    int sock_udp = socket(AF_INET, SOCK_DGRAM, 0);
    int reuse_udp = 1;

    setsockopt(sock_udp, SOL_SOCKET, SO_REUSEADDR, &reuse_udp, sizeof(reuse_udp));
    memset((char*)&my_addr_udp, 0, sizeof(my_addr_udp));
    my_addr_udp.sin_addr.s_addr = INADDR_ANY;
    my_addr_udp.sin_family = AF_INET;
    my_addr_udp.sin_port = htons(atoi(argv[2]));
    bind(sock_udp, (struct sockaddr*)&my_addr_udp, sizeof(my_addr_udp));
    listen(sock_udp, 5);


    /** Defining UDP DATA socket **/
    struct sockaddr_in data_msg_udp;
    int sock_udp_data = socket(AF_INET, SOCK_DGRAM, 0);


    setsockopt(sock_udp_data, SOL_SOCKET, SO_REUSEADDR, &reuse_udp, sizeof(reuse_udp));
    memset((char*)&data_msg_udp, 0, sizeof(data_msg_udp));
    data_msg_udp.sin_addr.s_addr = INADDR_ANY;
    data_msg_udp.sin_family = AF_INET;
    data_msg_udp.sin_port = 0;

    int data_port = 1234;
    
    data_msg_udp.sin_port = htons(data_port);


    /** Server running ... **/
    while(1) {
        struct sockaddr_in my_addr_udp_client;
        fd_set im_socket;
        FD_SET(sock_udp, &im_socket);
        printf("\n ======= \n");
        select(5, &im_socket, NULL, NULL, NULL);

        printf("Receving UDP message \n");
        printf("** port : %d \n", ntohs(my_addr_udp.sin_port));

        // waiting to receive SYN
        char buffer_udp[MSG_LEN];
        unsigned int udp_size = sizeof(my_addr_udp);
        do {recvfrom(sock_udp, &buffer_udp, MSG_LEN, 0, (struct sockaddr*)&my_addr_udp, &udp_size);
            printf("Waiting for SYN... Reading : %s \n", buffer_udp);
        } while (strcmp(buffer_udp, "SYN") != 0 );


        // Sending SYN_ACK
        bind(sock_udp_data, (struct sockaddr*)&data_msg_udp, sizeof(data_msg_udp));
        char buffer_udp_syn_ack[11];
        sprintf(buffer_udp_syn_ack, "SYN-ACK%d", ntohs(data_msg_udp.sin_port));
        printf("sending SYN_ACK ...\n");
        sendto(sock_udp, buffer_udp_syn_ack, strlen(buffer_udp_syn_ack)+1, 0, (struct sockaddr*)&my_addr_udp, sizeof(my_addr_udp));


        // Receiving ACK
        char buffer_udp_ack[MSG_LEN];
        unsigned int udp_size_ack = sizeof(my_addr_udp);
        do{recvfrom(sock_udp, &buffer_udp_ack, MSG_LEN, 0, (struct sockaddr*)&my_addr_udp, &udp_size_ack);
            printf("Waiting for ACK... Reading : %s \n", buffer_udp_ack);
        } while(strcmp(buffer_udp_ack, "ACK") != 0);


        printf("ACK exchanges completed succesfully \n");
        printf("\n** Switching to port : %d \n", ntohs(data_msg_udp.sin_port));
        // Receiving data message 'hello, I'm a useful message'
        char buffer_udp_msg[MSG_LEN_USEFUL];
        recvfrom(sock_udp_data, &buffer_udp_msg, MSG_LEN_USEFUL, 0, (struct sockaddr*)&data_msg_udp, &udp_size_ack);
        printf("reading : %s \n", buffer_udp_msg);

        // Sending the jpeg file
        int n = 0 ;
        //char buffer[SIZE];
        FILE* fp = fopen("Britse-Korthaar.jpg", "r");
        int size = fseek(fp, 0L, SEEK_END);
        long int res = ftell(fp);
        printf("Size file %d \n", res);

        //Going at the begening of the file to transmmit the begining
        fseek(fp, 0L, SEEK_SET);
        char buffer_file[SIZE];
        int sequence_number = 1;

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000;

        int chunk_file;
        char seq_num[1032];

        // While not end of file
        while (!feof(fp))
        {   
            // Reading the file & sending a part
            chunk_file = fread(buffer_file, 1, SIZE, fp);
            sprintf(seq_num, "%06d", sequence_number);
            memcpy(seq_num+6, buffer_file, chunk_file);
            sendto(sock_udp_data, seq_num, SIZE, 0, (struct sockaddr*)&my_addr_udp, sizeof(my_addr_udp));
            printf("sending : %d ", sequence_number);
            
            n = recvfrom(sock_udp_data, buffer_file, SIZE, 0, (struct sockaddr*)&data_msg_udp, &udp_size_ack);

            if (setsockopt(sock_udp_data, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
            perror("Error");
            }

            //all good
            if (n > 0)
            {
                printf("received : %s \n", buffer_file);
            }

            // if error in receiving (< 0)
            else 
            {
                do{
                    printf("Error when receiving (%d), sending again %d\n", n, sequence_number);
                    sendto(sock_udp_data, seq_num, SIZE, 0, (struct sockaddr*)&my_addr_udp, sizeof(my_addr_udp));
                    n = recvfrom(sock_udp_data, buffer_file, SIZE, 0, (struct sockaddr*)&data_msg_udp, &udp_size_ack);
                    printf("received BIS : %s \n", buffer_file);
                } while(n < 0);
            }

            // Making sure its the correct ACK
            if (atoi(&buffer_file[3]) != sequence_number)
                printf("expected received msg : %d, but received : %d SOS EVERYONE MUST PANIC \n",sequence_number, atoi(&buffer_file[3]) );
                
            sequence_number += 1;
            
        }
        fclose(fp);
        // Sending it's the end of the file to the client
        char *file_ended = "FIN";
        sendto(sock_udp, file_ended, strlen(file_ended)+1, 0, (struct sockaddr*)&my_addr_udp, sizeof(my_addr_udp));
        printf("**File sent : EOF**\n");

    } // end while(1)-> server stoped running 
}
    
    