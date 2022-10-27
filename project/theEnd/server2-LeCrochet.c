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

#define MSG_LEN 10
#define MSG_LEN_USEFUL 100
#define SIZE 1466
#define ACK_BUFFER_SIZE 2
#define BUFFER 25

pthread_t t_send;
pthread_t t_receive;

// data with mutex
int credit = BUFFER;
int received_ack = 0;
int fast_r = 0;
int flag_STOP = 0;

// def of mutex
pthread_mutex_t lock;

int areSame(int arr[], int n) {
  int first = arr[0];

  for (int i = 1; i < n; i++)
    if (arr[i] != first)
      return 0;
  return 1;
}

struct data_thread {
  struct sockaddr_in *addr_ptr;
  int sock_in;
  char *file_name;
};

int create_sock(struct sockaddr_in *addr_ptr, int port) {
  int valid = 1;
  int sock = socket(AF_INET, SOCK_DGRAM, 0);

  if (sock < 0) {
    perror("Cannot create socket\n");
    return -1;
  }

  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &valid, sizeof(int));

  addr_ptr->sin_family = AF_INET;
  addr_ptr->sin_port = htons(port);
  addr_ptr->sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(sock, (struct sockaddr *)addr_ptr, sizeof(struct sockaddr_in)) == -1) {
    perror("Bind failed\n");
    close(sock);
    return -1;
  }
  return sock;
}

void *thread_send(void *data) {

  struct data_thread *info = data;

  int sock = info->sock_in;
  struct sockaddr_in *addr_ptr = info->addr_ptr;
  char *file_name = info->file_name;

  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 10000;

  fd_set rtimeout;
  FD_ZERO(&rtimeout);
  FD_SET(sock, &rtimeout);

  int chunk_file; // size of data chunk
  char data_send[1472];
  char circular_buffer[BUFFER][SIZE] = {0}; // circular buffer with window size
  int sequence_number = 0, local = 0;

  FILE *fp = fopen(file_name, "r");
  int size = fseek(fp, 0L, SEEK_SET);

  while (!feof(fp)) {
    pthread_mutex_lock(&lock);
    local = received_ack; // store present ack on local variable of thread
    pthread_mutex_unlock(&lock);
    if (credit > 0) {
      if (!feof(fp) && (sequence_number == local)) { // present ack = last sequence of file read
        sequence_number++;
        sprintf(data_send, "%06d", sequence_number);
        chunk_file = fread((char *)&circular_buffer[(sequence_number - 1) % BUFFER], 1, SIZE, fp); // read new sequence
        memcpy(data_send + 6, (char *)&circular_buffer[(sequence_number - 1) % BUFFER], chunk_file);
        sendto(sock, data_send, chunk_file + 6, 0, (struct sockaddr *)addr_ptr, sizeof(struct sockaddr_in)); // send sequence
        pthread_mutex_lock(&lock);
        credit--; // decrease 1 credit
        pthread_mutex_unlock(&lock);
      }
    }

    if (fast_r == 1) { // fast restransmit flag on
      sprintf(data_send, "%06d", local + 1);
      memcpy(data_send + 6, (char *)&circular_buffer[(local) % BUFFER], chunk_file);
      sendto(sock, data_send, chunk_file + 6, 0, (struct sockaddr *)addr_ptr, sizeof(struct sockaddr_in)); // send next sequence after present ack
      pthread_mutex_lock(&lock);
      fast_r = 0; // flag off
      pthread_mutex_unlock(&lock);
    }

    int ret = select(sock + 1, &rtimeout, NULL, NULL, &tv); // select on socket with timeout
    tv.tv_sec = 0;
    tv.tv_usec = 700;
    if (ret == 0) { // if timeout
      if (sequence_number == local) { // if same ack
        pthread_mutex_lock(&lock);
        credit++; // give credit, next lap will send a new sequence of file
        pthread_mutex_unlock(&lock);
        usleep(10); // give sleep to let recvfrom thread get ack
      } else {
        sprintf(data_send, "%06d", local + 1);
        memcpy(data_send + 6, (char *)&circular_buffer[(local) % BUFFER], chunk_file);
        sendto(sock, data_send, chunk_file + 6, 0, (struct sockaddr *)addr_ptr, sizeof(struct sockaddr_in)); // send next sequence after present ack
      }
    }
  }
  fclose(fp);
  flag_STOP = 1; // flag stop recvfrom
  return NULL;
}

void *thread_receive(void *data) {
  struct data_thread *info = data; // getting data of main into socket

  int sock = info->sock_in;
  struct sockaddr_in *addr_ptr = info->addr_ptr;
  char *file_name = info->file_name;

  char buffer_file[SIZE]; // init buffer of recvfrom
  int buffer_ACK[ACK_BUFFER_SIZE] = {0}; // init ack buffer for fast restransmit
  int last_ACK = 0, actu_ACK = 0; // init actual ack and last ack

  int ld = sizeof(struct sockaddr *);
  while (1) {
    recvfrom(sock, buffer_file, SIZE, 0, (struct sockaddr *)addr_ptr, &ld); // recvfrom client data ack

    last_ACK = actu_ACK; // sorting last ack
    actu_ACK = atoi(&buffer_file[3]); // storing new data

    if (actu_ACK < last_ACK) { // condition to get higher ack
      actu_ACK = last_ACK;
    }

    for (size_t k = 0; k < ACK_BUFFER_SIZE; k++) { // storing last 4 ack's
      buffer_ACK[k] = buffer_ACK[k + 1];
    }
    buffer_ACK[ACK_BUFFER_SIZE - 1] = actu_ACK;

    if (areSame(buffer_ACK, ACK_BUFFER_SIZE)) { // if all 4 ack's are same
      pthread_mutex_lock(&lock);
      fast_r = 1; // start fast restransmit
      pthread_mutex_unlock(&lock);
    }

    pthread_mutex_lock(&lock);
    received_ack = actu_ACK;
    pthread_mutex_unlock(&lock);

    pthread_mutex_lock(&lock);
    credit = credit + (actu_ACK - last_ACK); // calcul new credit
    pthread_mutex_unlock(&lock);

    if (flag_STOP == 1) {
      break; // end if flag stop
    }
  }
  return NULL;
}

int main(int argc, char *argv[]) {
  time_t start, end;
  int port = 1500;
  /** Server running ... **/
  while (1) {
    /** Defining UDP CONTROL socket **/
    struct sockaddr_in my_addr_udp;
    int reuse_udp = 1;
    int sock_udp = create_sock(&my_addr_udp, atoi(argv[1]));

    struct sockaddr_in my_addr_udp_client;
    fd_set im_socket;
    FD_SET(sock_udp, &im_socket);

    // waiting to receive SYN
    char buffer_udp[MSG_LEN];
    unsigned int udp_size = sizeof(my_addr_udp);
    do {
      recvfrom(sock_udp, &buffer_udp, MSG_LEN, 0, (struct sockaddr *)&my_addr_udp, &udp_size);
    } while (strcmp(buffer_udp, "SYN") != 0);

    port = port + 1;

    /** Defining UDP DATA socket **/
    struct sockaddr_in data_msg_udp;
    int sock_udp_data = create_sock(&data_msg_udp, port);

    // Sending SYN_ACK
    char buffer_udp_syn_ack[11];
    sprintf(buffer_udp_syn_ack, "SYN-ACK%d", ntohs(data_msg_udp.sin_port));
    sendto(sock_udp, buffer_udp_syn_ack, strlen(buffer_udp_syn_ack) + 1, 0, (struct sockaddr *)&my_addr_udp, sizeof(my_addr_udp));

    // Receiving ACK
    char buffer_udp_ack[MSG_LEN];
    unsigned int udp_size_ack = sizeof(my_addr_udp);
    do {
      recvfrom(sock_udp, &buffer_udp_ack, MSG_LEN, 0, (struct sockaddr *)&my_addr_udp, &udp_size_ack);
    } while (strcmp(buffer_udp_ack, "ACK") != 0);


    close(sock_udp);

    char buffer_udp_msg[MSG_LEN_USEFUL];
    recvfrom(sock_udp_data, &buffer_udp_msg, MSG_LEN_USEFUL, 0, (struct sockaddr *)&data_msg_udp, &udp_size_ack);
    struct data_thread data;

    data.addr_ptr = &data_msg_udp;
    data.sock_in = sock_udp_data;
    data.file_name = buffer_udp_msg;

    pthread_create(&t_send, NULL, thread_send, &data);
    pthread_create(&t_receive, NULL, thread_receive, &data);
    pthread_join(t_send, NULL);
    pthread_join(t_receive, NULL);

    pthread_mutex_destroy(&lock);

    char *file_ended = "FIN";
    int try;
    for (try= 1; try <= 5; try++) {
        sendto(sock_udp, file_ended, strlen(file_ended) + 1, 0, (struct sockaddr *)&my_addr_udp, sizeof(my_addr_udp));

        (5*try);
    }
    //sleep(1);
    sendto(sock_udp_data, file_ended, strlen(file_ended) + 1, 0, (struct sockaddr *)&my_addr_udp, sizeof(my_addr_udp));
    //printf("**File sent : EOF**\n");

    // FERMER SOCKET DONNEE
    close(sock_udp_data);
  }
}
