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

#define MSG_LEN 60
#define MSG_LEN_USEFUL 700
#define SIZE 1466
#define ACK_BUFFER_SIZE 2
#define BUFFER 30

pthread_t t_send;
pthread_t t_receive;

// VALEURS A SURVEILLER AVEC MUTEX
int credit = BUFFER;
int received_ack = 0;
int flag_STOP = 0;

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

// FONCTION CREATION DE SOCKET
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

// FONCTION THREAD SEND
void *thread_send(void *data) {
  printf("Entering thread send\n");

  struct data_thread *info = data;

  int sock = info->sock_in;
  struct sockaddr_in *addr_ptr = info->addr_ptr;
  char *file_name = info->file_name;

  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 100000;

  fd_set rtimeout;
  FD_ZERO(&rtimeout);
  FD_SET(sock, &rtimeout);
  select(sock, &rtimeout, NULL, NULL, &tv);

  int chunk_file;
  char data_send[1472];
  char circular_buffer[BUFFER][SIZE] = {0};
  int sequence_number = 0, local = 0;

  // OUVRIR LE FICHIER
  FILE *fp = fopen(file_name, "r");
  int size = fseek(fp, 0L, SEEK_SET);

  while (!feof(fp)) {
    pthread_mutex_lock(&lock);
    local = received_ack;
    pthread_mutex_unlock(&lock);
    if (credit > 0) {
      printf("local : %d, seq : %d\n",sequence_number,local);
      if (!feof(fp) && (sequence_number == local)) {
        sequence_number++;
        sprintf(data_send, "%06d", sequence_number);
        chunk_file = fread((char *)&circular_buffer[(sequence_number - 1) % BUFFER], 1, SIZE, fp);
        memcpy(data_send + 6, (char *)&circular_buffer[(sequence_number - 1) % BUFFER], chunk_file);

        // ENVOI
        sendto(sock, data_send, chunk_file + 6, 0, (struct sockaddr *)addr_ptr, sizeof(struct sockaddr_in));

        // INCREMENTATION DU NUMERO DE SEQUENCE
        pthread_mutex_lock(&lock);
        credit--;
        pthread_mutex_unlock(&lock);
      }
    }

    int ret = select(sock + 1, &rtimeout, NULL, NULL, &tv);
    if (ret == 0) {
      // printf("TIMEOUT\n");
      if (sequence_number == local) {
        printf("ajout credit\n");
        pthread_mutex_lock(&lock);
        credit++;
        pthread_mutex_unlock(&lock);
        usleep(10);
      } else {
        sprintf(data_send, "%06d", local+1);
        memcpy(data_send + 6, (char *)&circular_buffer[(local) % BUFFER], chunk_file);
        sendto(sock, data_send, chunk_file + 6, 0, (struct sockaddr *)addr_ptr, sizeof(struct sockaddr_in));
      }
    }
  }

  fclose(fp);
  flag_STOP = 1;
  return NULL;
}

// FONCTION THREAD RECEIVE
void *thread_receive(void *data) {
  struct data_thread *info = data;

  int sock = info->sock_in;
  struct sockaddr_in *addr_ptr = info->addr_ptr;
  char *file_name = info->file_name;

  printf("Entering thread receive\n");
  char buffer_file[SIZE];

  int buffer_ACK[ACK_BUFFER_SIZE] = {0};
  int last_ACK = 0, actu_ACK = 0, buffer = 0, temp = 0;
  int ld = sizeof(struct sockaddr *);
  while (1) {
    recvfrom(sock, buffer_file, SIZE, 0, (struct sockaddr *)addr_ptr, &ld);

    // STOCKAGE DU DERNIER ACK
    last_ACK = actu_ACK;
    // AJOUT DU NOUVEL ACK
    actu_ACK = atoi(&buffer_file[3]);

    // COMPARAISON SI NOUVEL ACK < ANCIEN ACK (pour toujours avoir le dernier ACK reçu dans received_ack)
    if (actu_ACK < last_ACK) {
      actu_ACK = last_ACK;
    }
    // printf("actu : %d, last : %d\n", actu_ACK, last_ACK);

    // PASSAGE DE actu_ACK AU THREAD SENDTO
    pthread_mutex_lock(&lock);
    received_ack = actu_ACK;
    pthread_mutex_unlock(&lock);

    // ACTUALISATION DU CREDIT
    pthread_mutex_lock(&lock);
    // if (credit < 30) {
    //   temp = credit + (actu_ACK - last_ACK);
    //   // printf("credit_t %d\n", actu_ACK - last_ACK);
    //   if (temp > 30) {
    //     temp = 30;
    //     credit = temp;
    //   } else {
    //     credit = temp;
    //   }
    // }
    credit = credit + (actu_ACK - last_ACK);
    pthread_mutex_unlock(&lock);

    if (flag_STOP == 1) {
      break;
    }
  }
  return NULL;
}

int main(int argc, char *argv[]) {
  time_t start, end;
  int port = 1500;
  /** Server running ... **/
  while (1) {
    printf("Entering server...");
    /** Defining UDP CONTROL socket **/
    struct sockaddr_in my_addr_udp;
    int reuse_udp = 1;
    int sock_udp = create_sock(&my_addr_udp, atoi(argv[2]));

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
    printf("portincr : %d\n", port);

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

    printf("ACK exchanges completed succesfully \n");

    // CREATION DE FORK DONNEE
    int x = fork();

    // SEPARATION PARENT/ENFANT
    if (x != 0) {
      close(sock_udp_data);
      continue;
    }

    // FERMER SOCKET ACCUEIL POUR ENFANT
    close(sock_udp);

    char buffer_udp_msg[MSG_LEN_USEFUL];
    // unsigned int udp_size_ack = sizeof(data_msg_udp);
    recvfrom(sock_udp_data, &buffer_udp_msg, MSG_LEN_USEFUL, 0, (struct sockaddr *)&data_msg_udp, &udp_size_ack);
    printf("reading : %s \n", buffer_udp_msg);
    struct data_thread data;

    data.addr_ptr = &data_msg_udp;
    data.sock_in = sock_udp_data;
    data.file_name = buffer_udp_msg;

    pthread_create(&t_send, NULL, thread_send, &data);
    pthread_create(&t_receive, NULL, thread_receive, &data);
    pthread_join(t_send, NULL);
    pthread_join(t_receive, NULL);

    pthread_mutex_destroy(&lock);
    sleep(1);
    char *file_ended = "FIN";
    sendto(sock_udp_data, file_ended, strlen(file_ended) + 1, 0, (struct sockaddr *)&my_addr_udp, sizeof(my_addr_udp));
    printf("**File sent : EOF**\n");

    // FERMER SOCKET DONNEE
    close(sock_udp_data);

    // FIN DE LA FORK CHILD
    exit(x);
  }
}
