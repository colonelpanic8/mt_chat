#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <pthread.h>
#include "bulletin.h"
#include "dicth.h"

#define BUF_SIZE 256
#define INIT_DICT_SIZE 5


dict_t *D;

//easier to use a global than to pack it into a struct as an argument
pthread_mutex_t D_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t waiting_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t waiting = PTHREAD_COND_INITIALIZER;

void reg(int socket, char *host);
void msg(int socket);
void del(int socket, char *host);


int process_post(int socket, char *input, char *host) {
  if(!strcmp(input, "reg")) {
    reg(socket, host);
  }
  if(!strcmp(input, "msg")) {
    msg(socket);
  }
  if(!strcmp(input, "del")) {
    del(socket, host);
  }
  return 0;
}

void msg(int socket) {
  hostport *tosend;
  char buffer[BUF_SIZE];
  int err, size;
  while(1) {
    recv_string(socket, buffer, 255);
    if(!strcmp(buffer, "end")) {
      close(socket);
      return;
    }
    recv_string(socket, buffer, 255);
    pthread_mutex_lock(&D_mutex);
    if(containsD(buffer, D)) {
      size = gethp(buffer, &tosend, D);
    } else {
      pthread_mutex_unlock(&D_mutex);
      size = 0;
    }
    pthread_mutex_unlock(&D_mutex);
    sprintf(buffer, "%d", size);
    send_string(socket, buffer);
    if(size) {
      write(socket, tosend, size*sizeof(hostport));
    }
  }
}

void reg(int socket, char *host) {
  char buffer[BUF_SIZE], *user, *port, *ip;

  //get strings, allocate heap memory so that they dont disappear
  recv_string(socket, buffer, 255);
  user = malloc(sizeof(char)*(strlen(buffer)+1));
  strcpy(user, buffer);

  recv_string(socket, buffer, 255);
  port = malloc(sizeof(char)*(strlen(buffer)+1));
  strcpy(port, buffer);

  ip = malloc(sizeof(char)*(strlen(host)+1));
  strcpy(ip, host);

  //add to dict and output
  pthread_mutex_lock(&D_mutex);
  insertD(user, ip, port,D);
  outputD(D);
  pthread_mutex_unlock(&D_mutex);
  printf("----------------------------------\n");
}

void del(int socket, char *host) {
  char user[BUF_SIZE], port[BUF_SIZE];

  //get strings, allocate heap memory so that they dont disappear
  recv_string(socket, user, 255);


  recv_string(socket, port, 255);


  //add to dict and output
  pthread_mutex_lock(&D_mutex);
  removeD(user, host, port,D);
  outputD(D);
  pthread_mutex_unlock(&D_mutex);
  printf("----------------------------------\n");
}



int receive_connection(int *listener) {
  char buffer[BUF_SIZE];
  char str[INET_ADDRSTRLEN];
  int connection, connect_result;
  //connect
  connect_result = bulletin_wait_for_connection(*listener, &connection);
  if (connect_result < 0) bulletin_exit(connect_result);

  //tell the main thread to make a new listening thread
  pthread_mutex_lock(&waiting_mutex);
  pthread_cond_signal(&waiting);
  pthread_mutex_unlock(&waiting_mutex);

  
  //gets connection ip address
  get_ip(connection, str);


  //receive and process post
  recv_string(connection,buffer,255);
  process_post(connection, buffer, str);
  close(connection);
  return 0;
}


int main(int argc, char **argv) {
  int *connection, listener;
  int connect_result;
  struct sockaddr_in address;
  pthread_t thread;
  char temp[INET_ADDRSTRLEN];
  D = newD(INIT_DICT_SIZE);
  
  connect_result = bulletin_set_up_listener(atoi(argv[1]),&listener);
  if (connect_result < 0) bulletin_exit(connect_result);

  while (1) {
    pthread_create(&thread, NULL, (void *(*)(void *))receive_connection, &listener);
    //this lock/unlock is probably unneedes since only this thread will access the condtion
    //but it is included as a matter of style
    pthread_mutex_lock(&waiting_mutex);
    pthread_cond_wait(&waiting, &waiting_mutex);
    pthread_mutex_unlock(&waiting_mutex);
  }
}
