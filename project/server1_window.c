#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#define MSG_LEN 5
#define MSG_LEN_USEFUL 100
#define PORT_DATA 1234
#define SIZE 1466
#define ACK_BUFFER_SIZE 3
#define BUFFER 10

pthread_t t_send;
pthread_t t_receive;
pthread_mutex_t lock;

// VALEURS A SURVEILLER AVEC MUTEX
int credit = 30;
int value_flag_ACK = 0;
int flag_STOP = 0;

int areSame(int arr[], int n)
{
    int first = arr[0];

    for (int i = 1; i < n; i++)
        if (arr[i] != first)
            return 0;
    return 1;
}

struct data_thread
{
    struct sockaddr_in *addr_ptr;
    int sock_in;
    char *file_name;
};

// FONCTION CREATION DE SOCKET
int create_sock(struct sockaddr_in *addr_ptr, int port)
{
    int valid = 1;
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock < 0)
    {
        perror("Cannot create socket\n");
        return -1;
    }

    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &valid, sizeof(int));

    addr_ptr->sin_family = AF_INET;
    addr_ptr->sin_port = htons(port);
    addr_ptr->sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (struct sockaddr *)addr_ptr, sizeof(struct sockaddr_in)) == -1)
    {
        perror("Bind failed\n");
        close(sock);
        return -1;
    }

    return sock;
}

// FONCTION THREAD SEND
void *thread_send(void *data)
{
    struct data_thread *info = data;
    int sock = info->sock_in;
    struct sockaddr_in *addr_ptr = info->addr_ptr;
    char *file_name = info->file_name;

    int ntimeout;
    const char *mode = "r";
    FILE *fp = fopen(file_name, mode);
    int size = fseek(fp, 0L, SEEK_END);
    long int res = ftell(fp);
    printf("Size file %d \n", res);

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;

    fd_set rtimeout;
    FD_ZERO(&rtimeout);
    FD_SET(sock, &rtimeout);

    int chunk_file;
    char data_send[1472];
    char buffer_file[SIZE];
    char circular_buffer[BUFFER][SIZE] = {0};
    int i, sequence_number = 0;

    memset(buffer_file, 0, sizeof(buffer_file));
    rewind(fp);
    while (!feof(fp))
    {
        while (credit > 0)
        {
            sequence_number++;
            sprintf(data_send, "%06d", sequence_number);
            if (!feof(fp)){
                //printf("A \n");
                chunk_file = fread((char *)&circular_buffer[sequence_number % BUFFER], 1, SIZE, fp);
                memcpy(data_send + 6, (char *)&circular_buffer[sequence_number % BUFFER], chunk_file);
                sendto(sock, data_send, chunk_file+6, 0, (struct sockaddr *)addr_ptr, sizeof(struct sockaddr_in));
            }
            pthread_mutex_lock(&lock);
            credit--;
            pthread_mutex_unlock(&lock);
        }

        int ret= select(sock+1, &rtimeout,NULL, NULL, &tv);







        if ((ret == 0) && (value_flag_ACK == 0)) {
            sprintf(data_send, "%06d", value_flag_ACK);
            memcpy(data_send + 6, (char *)&circular_buffer[value_flag_ACK % BUFFER], chunk_file);
            sendto(sock, data_send, SIZE, 0, (struct sockaddr *)addr_ptr, sizeof(struct sockaddr_in));
        }


        pthread_mutex_lock(&lock);
        if (value_flag_ACK != 0) {
            //printf("C \n");
            sprintf(data_send, "%06d", value_flag_ACK+1);
            memcpy(data_send + 6, (char *)&circular_buffer[(value_flag_ACK+1) % BUFFER], chunk_file);
            sendto(sock, data_send, SIZE, 0, (struct sockaddr *)addr_ptr, sizeof(struct sockaddr_in));
            sprintf(data_send, "%06d", value_flag_ACK+2);
            memcpy(data_send + 6, (char *)&circular_buffer[(value_flag_ACK+2) % BUFFER], chunk_file);
            sendto(sock, data_send, SIZE, 0, (struct sockaddr *)addr_ptr, sizeof(struct sockaddr_in));
            sprintf(data_send, "%06d", value_flag_ACK+3);
            memcpy(data_send + 6, (char *)&circular_buffer[(value_flag_ACK+3) % BUFFER], chunk_file);
            sendto(sock, data_send, SIZE, 0, (struct sockaddr *)addr_ptr, sizeof(struct sockaddr_in));
            /*sprintf(data_send, "%06d", value_flag_ACK+4);
            memcpy(data_send + 6, (char *)&circular_buffer[(value_flag_ACK+4) % BUFFER], chunk_file);
            sendto(sock, data_send, SIZE, 0, (struct sockaddr *)addr_ptr, sizeof(struct sockaddr_in));*/
            sleep(0.005);
        }
        pthread_mutex_unlock(&lock);
    }
    fclose(fp);
    flag_STOP = 1;
    return NULL;
}

// FONCTION THREAD RECEIVE
void *thread_receive(void *data)
{
    struct data_thread *info = data;                     

    int sock = info->sock_in;
    struct sockaddr_in *addr_ptr = info->addr_ptr;
    char *file_name = info->file_name;
    char buffer_file[SIZE];

    int buffer_ACK[ACK_BUFFER_SIZE] = {0};
    int last_ACK = 0, actu_ACK = 0, temp = 0;
    int ld = sizeof(struct sockaddr *);

    while (1)
    {   
        recvfrom(sock, buffer_file, SIZE, 0, (struct sockaddr *)addr_ptr, &ld);
        /*for (int i = 0; i < ACK_BUFFER_SIZE - 1; i++)
        {
            buffer_ACK[i] = buffer_ACK[i + 1];
        }
        //printf("buffer_file %d \n", buffer_file[3]);
        buffer_ACK[ACK_BUFFER_SIZE - 1] = atoi(&buffer_file[3]);*/


        int init_value_stock = 0, init_val = 0;
        int new_val = atoi(&buffer_file[3]);
        init_value_stock = init_val;
        if (new_val > init_val){
            init_val = new_val;
        }

        /*for (int j = 0; j < ACK_BUFFER_SIZE; j++)
        {
            if (temp < buffer_ACK[j])
            {
                temp = buffer_ACK[j];
            }
        }*/

        /*last_ACK = actu_ACK;
        printf("last_ACK %d temp %d \n", last_ACK, temp);
        actu_ACK = temp;*/

        pthread_mutex_lock(&lock);
        if (credit < 30)
        {
            credit = credit + (init_val - init_value_stock);
        }
        pthread_mutex_unlock(&lock);

        int first = buffer_ACK[0];

        /*if (areSame(buffer_ACK, ACK_BUFFER_SIZE) == 1)
        {
            pthread_mutex_lock(&lock);
            value_flag_ACK = actu_ACK;
            pthread_mutex_unlock(&lock);
        }*/

        pthread_mutex_lock(&lock);
        value_flag_ACK = init_val;
        pthread_mutex_unlock(&lock);

       /* else
        {
            pthread_mutex_lock(&lock);
            value_flag_ACK = 0;
            pthread_mutex_unlock(&lock);
        }*/

        if (flag_STOP == 1)
        {
            break;
        }
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    /** Defining UDP CONTROL socket **/
    struct sockaddr_in my_addr_udp;
    int reuse_udp = 1;
    int sock_udp = create_sock(&my_addr_udp, atoi(argv[2]));

    while (1)
    {

        /** Defining UDP DATA socket **/
        struct sockaddr_in data_msg_udp;
        int sock_udp_data = create_sock(&data_msg_udp, PORT_DATA);

        char buffer_udp[MSG_LEN];
        unsigned int udp_size = sizeof(my_addr_udp);
        do {recvfrom(sock_udp, &buffer_udp, MSG_LEN, 0, (struct sockaddr *)&my_addr_udp, &udp_size);
        } while (strcmp(buffer_udp, "SYN") != 0);

        bind(sock_udp_data, (struct sockaddr *)&data_msg_udp, sizeof(data_msg_udp));
        char buffer_udp_syn_ack[11];
        sprintf(buffer_udp_syn_ack, "SYN-ACK%d", ntohs(data_msg_udp.sin_port));
        sendto(sock_udp, buffer_udp_syn_ack, strlen(buffer_udp_syn_ack) + 1, 0, (struct sockaddr *)&my_addr_udp, sizeof(my_addr_udp));

        char buffer_udp_ack[MSG_LEN];
        unsigned int udp_size_ack = sizeof(my_addr_udp);
        do {recvfrom(sock_udp, &buffer_udp_ack, MSG_LEN, 0, (struct sockaddr *)&my_addr_udp, &udp_size_ack);
        } while (strcmp(buffer_udp_ack, "ACK") != 0);

        char buffer_udp_msg[MSG_LEN_USEFUL];
        recvfrom(sock_udp_data, &buffer_udp_msg, MSG_LEN_USEFUL, 0, (struct sockaddr *)&data_msg_udp, &udp_size_ack);
        // printf("reading : %s \n", buffer_udp_msg);

        struct data_thread data;
        data.addr_ptr = &data_msg_udp;
        data.sock_in = sock_udp_data;
        data.file_name = buffer_udp_msg;

        pthread_create(&t_send, NULL, thread_send, &data);
        pthread_create(&t_receive, NULL, thread_receive, &data);
        pthread_join(t_send, NULL);
        pthread_join(t_receive, NULL);

        char *file_ended = "FIN";
        int try;
        for (try= 1; try <= 10; try++) {
            sendto(sock_udp, file_ended, strlen(file_ended) + 1, 0, (struct sockaddr *)&my_addr_udp, sizeof(my_addr_udp));
            usleep(5*try);
        }
        printf("EOF \n" );
    }
}

/*
MEMO :
w/ window : a 1.6Mo : 22 to 32sec

*/