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
#include <time.h>

#define MSG_LEN 5
#define MSG_LEN_USEFUL 100
#define PORT_DATA 1234
#define SIZE 1466

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
    data_msg_udp.sin_port = htons(PORT_DATA);

    time_t start,end;

    while(1) {
        char buffer_udp[MSG_LEN];
        unsigned int udp_size = sizeof(my_addr_udp);
        do {recvfrom(sock_udp, &buffer_udp, MSG_LEN, 0, (struct sockaddr*)&my_addr_udp, &udp_size);
        } while (strcmp(buffer_udp, "SYN") != 0 );

        bind(sock_udp_data, (struct sockaddr*)&data_msg_udp, sizeof(data_msg_udp));
        char buffer_udp_syn_ack[11];
        sprintf(buffer_udp_syn_ack, "SYN-ACK%d", ntohs(data_msg_udp.sin_port));
        sendto(sock_udp, buffer_udp_syn_ack, strlen(buffer_udp_syn_ack)+1, 0, (struct sockaddr*)&my_addr_udp, sizeof(my_addr_udp));

        char buffer_udp_ack[MSG_LEN];
        unsigned int udp_size_ack = sizeof(my_addr_udp);
        do{recvfrom(sock_udp, &buffer_udp_ack, MSG_LEN, 0, (struct sockaddr*)&my_addr_udp, &udp_size_ack);
        } while(strcmp(buffer_udp_ack, "ACK") != 0);

        char buffer_udp_msg[MSG_LEN_USEFUL];
        recvfrom(sock_udp_data, &buffer_udp_msg, MSG_LEN_USEFUL, 0, (struct sockaddr*)&data_msg_udp, &udp_size_ack);
        //printf("reading : %s \n", buffer_udp_msg);

        int n = 0 ;
        FILE* fp = fopen(buffer_udp_msg, "r");
        int size = fseek(fp, 0L, SEEK_END);
        long int res = (ftell(fp))/1000;
        printf("Size file %f Ko\n", res);

        fseek(fp, 0L, SEEK_SET);
        char buffer_file[SIZE];
        int sequence_number = 1;

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000;

        int chunk_file;
        char seq_num[1472];
        
        start = clock();
        while (!feof(fp))
        {   
            chunk_file = fread(buffer_file, 1, SIZE, fp);
            sprintf(seq_num, "%06d", sequence_number);
            memcpy(seq_num+6, buffer_file, chunk_file);
            sendto(sock_udp_data, seq_num, SIZE, 0, (struct sockaddr*)&my_addr_udp, sizeof(my_addr_udp));
            //printf("sending : %d ", sequence_number);
            
            n = recvfrom(sock_udp_data, buffer_file, SIZE, 0, (struct sockaddr*)&data_msg_udp, &udp_size_ack);

            if (setsockopt(sock_udp_data, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
            perror("Error");
            }

            /*if (n > 0)
                printf("received : %s \n", buffer_file);*/

            else 
            {
                do{
                    sendto(sock_udp_data, seq_num, SIZE, 0, (struct sockaddr*)&my_addr_udp, sizeof(my_addr_udp));
                    n = recvfrom(sock_udp_data, buffer_file, SIZE, 0, (struct sockaddr*)&data_msg_udp, &udp_size_ack);
                    //printf("received BIS : %s \n", buffer_file);
                } while(n < 0);
            }

            if (atoi(&buffer_file[3]) != sequence_number)
                printf("expected received msg : %d, but received : %d SOS EVERYONE MUST PANIC \n",sequence_number, atoi(&buffer_file[3]) );
            sequence_number += 1;
            
        }
        fclose(fp);
        char *file_ended = "FIN";
        sendto(sock_udp, file_ended, strlen(file_ended)+1, 0, (struct sockaddr*)&my_addr_udp, sizeof(my_addr_udp));

        end = clock();
        double duration = (((double)end - start)/CLOCKS_PER_SEC)*100;
        printf("**File sent : EOF**\n");
        printf("Time taken to execute in sec : %f \n", duration);

        int sos = res/duration;

        printf("speed of light : %.6f ko/s\n", sos);

    } // end while(1)-> server stoped running 
}


/*
MEMO : 
w/ window : a 1.6Mo : 22 to 32sec (entre 0.07 et 0.15 Mo/s)

*/