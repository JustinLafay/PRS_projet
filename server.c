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

struct segment {
    size_t size;
    unsigned short number;
    char data[SIZE];
};


int main(int argc, char* argv[])
{   
    /** Defining TCP socket **/
    struct sockaddr_in my_addr;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    int reuse = 1;

    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    memset((char*)&my_addr, 0, sizeof(my_addr));
    my_addr.sin_addr.s_addr = INADDR_ANY;
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(atoi(argv[1]));

    bind(sock, (struct sockaddr*)&my_addr, sizeof(my_addr));
    listen(sock, 5);
    
    printf("socket : %d \n", sock);
    if (sock == 0){
        printf("Return value is : 0");
    }
    if (sock <= 0){
        printf("SOS ! Return value is : %d", sock);
        return 1;
    }
    printf("argv2 for TCP port : %d \n", atoi(argv[2]));
    printf("argv3 for UDP control port : %d \n", atoi(argv[3]));
    printf("argv3 for UDP data port : %d \n", atoi(argv[4]));


    /** Defining UDP CONTROL socket **/
    struct sockaddr_in my_addr_udp;
    int sock_udp = socket(AF_INET, SOCK_DGRAM, 0);
    int reuse_udp = 1;

    setsockopt(sock_udp, SOL_SOCKET, SO_REUSEADDR, &reuse_udp, sizeof(reuse_udp));
    memset((char*)&my_addr_udp, 0, sizeof(my_addr_udp));
    my_addr_udp.sin_addr.s_addr = INADDR_ANY;
    my_addr_udp.sin_family = AF_INET;
    my_addr_udp.sin_port = htons(atoi(argv[3]));
    bind(sock_udp, (struct sockaddr*)&my_addr_udp, sizeof(my_addr_udp));
    listen(sock_udp, 5);


    /** Defining UDP DATA socket **/
    struct sockaddr_in data_msg_udp;
    int sock_udp_data = socket(AF_INET, SOCK_DGRAM, 0);

    setsockopt(sock_udp_data, SOL_SOCKET, SO_REUSEADDR, &reuse_udp, sizeof(reuse_udp));
    memset((char*)&data_msg_udp, 0, sizeof(data_msg_udp));
    data_msg_udp.sin_addr.s_addr = INADDR_ANY;
    data_msg_udp.sin_family = AF_INET;
    data_msg_udp.sin_port = htons(atoi(argv[4]));

    
    /** Server running ... **/
    int acceptation;
    socklen_t size= sizeof(my_addr);
    while(1) {
        struct sockaddr_in my_addr_udp_client;
        fd_set im_socket;
        FD_SET(sock, &im_socket);
        FD_SET(sock_udp, &im_socket);
        printf("\n ======= \n");
        select(5, &im_socket, NULL, NULL, NULL);

        /** If socket is TCP **/
        if (FD_ISSET(sock, &im_socket)){
            printf("Receving TCP message \n");
            acceptation = accept(sock, (struct sockaddr*)&my_addr, &size);
            printf("I m accpet value : %d \n", acceptation);
            printf("numero port 2 %d\n", ntohs(my_addr.sin_port));

            if (acceptation < 0) {
            perror("erreur accept");
            }   

            int Fork_val;
            Fork_val = fork();

            if (Fork_val == 0){
                printf("Fork fils: %d \n", Fork_val);
                char *buffer = "hey";
                write(acceptation, buffer, strlen(buffer));

                char sos[60];
                read(acceptation, sos, 60*sizeof(char));
                printf("reading : %s \n", sos);
                exit(0);
            }

            else{
                printf("Fork père: %d \n", Fork_val);
                close(acceptation);
            } 
        }

        /** If socket is UDP **/
        if (FD_ISSET(sock_udp, &im_socket)){
            printf("Receving UDP message \n");
            printf("** port : %d \n", ntohs(my_addr_udp.sin_port));

            // waiting to receive SYN
            char buffer_udp[MSG_LEN];
            unsigned int udp_size = sizeof(my_addr_udp);
            do {recvfrom(sock_udp, &buffer_udp, MSG_LEN, 0, (struct sockaddr*)&my_addr_udp, &udp_size);
                printf("Waiting for SYN... Reading : %s \n", buffer_udp);
            } while (strcmp(buffer_udp, "SYN") != 0 );
        

            /*char* data;
            int nb_users;
            for (nb_users = 8001; nb_users < 8011; ++nb_users)
            {
                char temp[10] = "";
                sprintf(temp, "SYN_ACK_%i ", nb_users);

                printf("Im trying things : %s \n", temp);
                // Prep port for data messages
                data_msg_udp.sin_port = htons(atoi(argv[4]));*/
                //data_msg_udp.sin_port = nb_users;
                //char *buffer_udp_syn_ack = temp;

            
            // Sending SYN_ACK
            bind(sock_udp_data, (struct sockaddr*)&data_msg_udp, sizeof(data_msg_udp));
            char *buffer_udp_syn_ack = "SYN_ACK";
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
            //unsigned int udp_size_ack = sizeof(data_msg_udp);
            recvfrom(sock_udp_data, &buffer_udp_msg, MSG_LEN_USEFUL, 0, (struct sockaddr*)&data_msg_udp, &udp_size_ack);
            printf("reading : %s \n", buffer_udp_msg);

            // Sending the jpeg file
            int n;
            //char buffer[SIZE];
            FILE* fp = fopen("Britse-Korthaar.jpg", "r");
            int size = fseek(fp, 0L, SEEK_END);
            long int res = ftell(fp);
            printf("Size file %d \n", res);

            //Going at the begening of the file to transmmit the begining
            fseek(fp, 0L, SEEK_SET);
            struct segment segm;
            segm.number = 0;
            segm.size = 0;
            char buffer_file[SIZE];

            // While not end of file
            while (!feof(fp))
            {   
                segm.size = fread(segm.data, 1, SIZE, fp);
                sendto(sock_udp_data, (char*)&segm, sizeof(struct segment), 0, (struct sockaddr*)&my_addr_udp, sizeof(my_addr_udp));

                char received_msg[11];
                sprintf(received_msg, "ACK_%d", segm.number);

                recvfrom(sock_udp_data, &buffer_file, SIZE, 0, (struct sockaddr*)&data_msg_udp, &udp_size_ack);
                printf("%s, %d \n", buffer_file, segm.size);

                if (strcmp(buffer_file, received_msg) != 0)
                    printf("soss");

                if (n == -1)
                {
                perror("[ERROR] sending data to the server.\n");
                exit(1);
                }
                segm.number += 1;
                
            }

            fclose(fp);
            // Sending it's the end of the file to the server
            char *file_ended = "I_AM_END_OF_FILE";
            sendto(sock_udp, file_ended, strlen(file_ended)+1, 0, (struct sockaddr*)&my_addr_udp, sizeof(my_addr_udp));
            printf("**File sent : EOF**\n");
            
        } // end UDP socket
    } // end while(1)-> server running 
    close(acceptation); 
}
    
    