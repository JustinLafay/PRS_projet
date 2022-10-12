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
        return 0;
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
            printf("port : %d \n", ntohs(my_addr_udp.sin_port));

            // waiting to receive SYN
            char buffer_udp[MSG_LEN];
            unsigned int udp_size = sizeof(my_addr_udp);
            recvfrom(sock_udp, &buffer_udp, MSG_LEN, 0, (struct sockaddr*)&my_addr_udp, &udp_size);
            printf("reading : %s \n", buffer_udp);
            int result;
            result = strcmp(buffer_udp, "SYN");
            // if SYN is received, sending syn_ack
            if (result == 0){

                char* data;
                int nb_users;

                /*for (nb_users = 8001; nb_users < 8011; ++nb_users)
                {
                    char temp[10] = "";
                    sprintf(temp, "SYN_ACK_%i ", nb_users);

                    printf("Im trying things : %s \n", temp);
                    // Prep port for data messages
                    data_msg_udp.sin_port = htons(atoi(argv[4]));*/
                    //data_msg_udp.sin_port = nb_users;
                    bind(sock_udp_data, (struct sockaddr*)&data_msg_udp, sizeof(data_msg_udp));


                    
                    //char *buffer_udp_syn_ack = temp;
                    char *buffer_udp_syn_ack = "SYN_ACK";
                    // Sending SYN_ACK
                    sendto(sock_udp, buffer_udp_syn_ack, strlen(buffer_udp_syn_ack)+1, 0, (struct sockaddr*)&my_addr_udp, sizeof(my_addr_udp));

                    char buffer_udp_ack[MSG_LEN];
                    unsigned int udp_size_ack = sizeof(my_addr_udp);
                    recvfrom(sock_udp, &buffer_udp_ack, MSG_LEN, 0, (struct sockaddr*)&my_addr_udp, &udp_size_ack);
                    printf("reading : %s \n", buffer_udp_ack);
                    int result_ack;
                    result_ack = strcmp(buffer_udp_ack, "ACK");
                    // If ACK is received, returning OK
                    if (result_ack == 0){
                        printf("ACK exchanges completed succesfully \n");
                        printf("port : %d \n", ntohs(data_msg_udp.sin_port));
                        // Receiving data message
                        char buffer_udp_msg[MSG_LEN_USEFUL];
                        unsigned int udp_size_ack = sizeof(data_msg_udp);
                        recvfrom(sock_udp_data, &buffer_udp_msg, MSG_LEN_USEFUL, 0, (struct sockaddr*)&data_msg_udp, &udp_size_ack);
                        printf("reading : %s \n", buffer_udp_msg);

                        char buffer_file_transmit[SIZE];
                        unsigned int file_transmit = sizeof(my_addr_udp);
                       
                
                        FILE* fp = fopen("image.jpeg", "w");

                        int compteur = 0;
                        while (compteur < 9790)
                        {
                            int n = recvfrom(sock_udp_data, &buffer_file_transmit, SIZE, 0, (struct sockaddr*)&file_transmit, &udp_size_ack);

                            compteur += n;
                            printf("SOS packages reveived : %d \n", n);
                            fwrite(buffer_file_transmit, 1, n, fp);
                            memset(buffer_file_transmit, 0, SIZE);
                        }

                        fclose(fp);
                        if (strcmp(buffer_file_transmit, "END") == 0)
                        {
                        break;
                        
                        }
                    }
                    else{
                        printf("SOS ! DID NOT RECEIVE ACK, EVERYONE MUST PANIC \n\n");
                        return 1;
                    }
                //} //end for
            }
            else return 1;

        }
    }
    close(acceptation); 
}
    
    