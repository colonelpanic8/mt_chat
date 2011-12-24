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

#define BUFFER_SIZE 256

pthread_mutex_t waiting_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t waiting = PTHREAD_COND_INITIALIZER;

#ifndef MAX_CONNECTIONS
#define MAX_CONNECTIONS 10
#endif

typedef struct _args {
  int argc;
  char **argv;
} args;

void unregister(int argc, char **argv);

int send_message(char *host, char *port, char *message, char *user) {
  int connection, listener;
  int connect_result;
  char buffer[BUFFER_SIZE], *myport;
  //argument check
  connect_result = bulletin_make_connection_with(host,atoi(port),&connection);
  if (connect_result < 0) {
    printf("connection to %s failed\n", host);
    return -1;
  }
  send(connection, message, (strlen(message)+1)*sizeof(char),0);
  send_string(connection, user);
  return 0;
}


void send_routine(int argc, char** argv) {
  hostport *results;
  char message[BUFFER_SIZE];
  int i, connection, connect_result;
  connect_result = bulletin_make_connection_with(argv[1],atoi(argv[2]),&connection);
  if (connect_result < 0){
    printf("sender connection to registry failed\n");
    bulletin_exit(connect_result);
  }
  send_string(connection, "msg");

  while(1) {
    printf("Type \"quit\" to exit or type something else to send a message\n");
    fgets(message, BUFFER_SIZE-1, stdin);
    if(!strcmp("quit\n", message)) {
      send_string(connection, "end");
      close(connection);
      unregister(argc, argv);
      exit(0);
    } else {
      
      results = malloc(sizeof(hostport)*MAX_CONNECTIONS);
      memset(results, 0, sizeof(hostport)*MAX_CONNECTIONS);
      send_string(connection, ""); //indicate that we want to continue by not
      //sending del
      printf("Enter the username to which you would like to send a message:\n");
      fgets(message, BUFFER_SIZE-1, stdin);
      message[strlen(message)-1] = '\0';  //remove the /n at the end
      
      send_string(connection, message); //send the name of the user 
      recv_string(connection, message, BUFFER_SIZE-1); //receive the number of locations
      i = atoi(message);
      if(i > 0) {
	//get list of conections
	read(connection, results, sizeof(hostport)*i);

	printf("Write your message:\n");
	fgets(message, BUFFER_SIZE-1, stdin);
	
	for(i=0; i<MAX_CONNECTIONS; i++) {
	  if(!strcmp("", results[i].host)) break;
	  printf("sent to: %s\n", results[i].host);
	  send_message(results[i].host, results[i].port, message, argv[3]);
	}
      } else {
	printf("That user is not online\n");
      }
    }
  }
}


int start_listening(char *port) {
  int connection, listener;
  int connect_result, len;
  char message_buffer[BUFFER_SIZE], str[BUFFER_SIZE], user[BUFFER_SIZE];
  connect_result = bulletin_set_up_listener(atoi(port),&listener);
  if (connect_result < 0) bulletin_exit(connect_result);
  // receive and handle a series of client posts, one connection at a time
  while (1) {
    // get the next client connection
    connect_result = bulletin_wait_for_connection(listener,&connection);
    if (connect_result < 0) bulletin_exit(connect_result);
    // receive that client's post
    get_ip(connection, str);
    if(recv(connection, message_buffer, BUFFER_SIZE*sizeof(char), 0) > 0) {
      recv_string(connection, user, BUFFER_SIZE-1);
      printf("%s@%s says: %s", user, str, message_buffer);
    }
    close(connection);
  }
}

void unregister(int argc, char **argv) {
  int connection, connect_result;
  char *myport;
  connect_result = bulletin_make_connection_with(argv[1],atoi(argv[2]),&connection);
  if (connect_result < 0) bulletin_exit(connect_result);
  if(argc == 5) myport = argv[4];
  else myport = argv[2];
  send_string(connection, "del");
  send_string(connection, argv[3]);
  send_string(connection, myport);
  close(connection);
}

int client(int argc, char **argv) {
  int connection, listener;
  int connect_result;
  char buffer[BUFFER_SIZE], *myport;
  //argument check
  if(argc < 4) {
    printf("The wrong number of arguments were provided.\n");
    exit(0);
  }
  connect_result = bulletin_make_connection_with(argv[1],atoi(argv[2]),&connection);
  if (connect_result < 0) bulletin_exit(connect_result);

  if(argc == 5) myport = argv[4];
  else myport = argv[2];
  send_string(connection, "reg");
  
  send_string(connection, argv[3]);
  send_string(connection, myport);

  close(connection);
  //tell the other thread to start sending routine
  pthread_mutex_lock(&waiting_mutex);
  pthread_cond_signal(&waiting);
  pthread_mutex_unlock(&waiting_mutex);

  if(!start_listening(myport)) {
    exit(-1);
  }
}

void client_start(args *args_) {
  client(args_->argc, args_->argv);
}

int main(int argc, char **argv) {
  pthread_t listen_thread;
  args *args_;
  char buffer[BUFFER_SIZE];
  args_ = malloc(sizeof(args));
  args_->argc = argc;
  args_->argv = argv;
  pthread_create(&listen_thread, NULL, (void *(*)(void*))client_start, (void *)args_);
  pthread_mutex_lock(&waiting_mutex);
  pthread_cond_wait(&waiting, &waiting_mutex);
  pthread_mutex_unlock(&waiting_mutex);
  send_routine(argc, argv);
}
