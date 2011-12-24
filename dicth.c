#include "prelude.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "dicth.h"
#include "prime.h"

int hash(char *name, int modulus) {
  int i;
  int h;
  for (i=0, h=0; name[i] != 0; i++) {
    h = (h * 256 + (int)name[i]) % modulus;
  }
  return h;
}

boolean containsD(char *name, dict_t *D) {
  int h;
  h = hash(name,D->size);
  while (D->table[h] && (strcmp(D->table[h]->user,name) || D->table[h]->state < -1)) {
    h = (h + 1) % D->size;
  }
  return (D->table[h] != NULL);
}

node *get(char *name, dict_t *D) {
  int h;
  h = hash(name,D->size);
  while (D->table[h] && (strcmp(D->table[h]->user,name) || D->table[h]->state < -1)) {
    h = (h + 1) % D->size;
  }
  return D->table[h];
}

int gethp(char *name, hostport **arr, dict_t *D) {
  int size, i;
  host *runner;
  node *results = get(name, D);
  hostport *array = *arr;
  if(results) {
    runner = results->hosts;
    size = 0;
    while(runner){
      runner = runner->next;
      size++;
    }
    array = malloc(sizeof(hostport)*size);
    runner = results->hosts;
    for(i=0; i<size; i++) {
      strcpy(array[i].port, runner->port);
      strcpy(array[i].host, runner->host_);
      runner = runner->next;
    }
    *arr = array;
    return size;
  }
  return -1;
}

int removeD(char  *name, char *hostname, char *port, dict_t *D) {
  int h;
  host *hosts, *prev;
  h = hash(name,D->size);
  while (D->table[h] && (strcmp(D->table[h]->user,name) || D->table[h]->state < -1)) {
    h = (h + 1) % D->size;
  }
  if (!D->table[h]) {
    return -1;
  } else {
    hosts = D->table[h]->hosts;
    if(!strcmp(hosts->host_, hostname) && !strcmp(hosts->port, port)) {
      free(hosts->host_);
      free(hosts->port);
      D->table[h]->hosts = hosts->next;
      if(!D->table[h]->hosts) {
	strcpy(D->table[h]->user, "deleted_user");
	D->table[h]->state = -1;
      }
      return 0;
    }
    prev = hosts;
    hosts = hosts->next;
    while(hosts) {
      if(!strcmp(hosts->host_, hostname) && !strcmp(hosts->port, port)) {
	free(hosts->host_);
	free(hosts->port);
	prev->next = hosts->next;
	free(hosts);
	return 0;
      }
      prev = prev->next;
      hosts = hosts->next;
    }
    return -1;
  }
}

void insertD(char *name, char *hostname, char *port, dict_t *D) {
  int h;
  host *new_host;
  if(D->num_elem >= (D->size/2)) {
    resize(D);
  }
  h = hash(name,D->size);
  while (D->table[h] && (strcmp(D->table[h]->user,name) || D->table[h]->state < -1)) {
    h = (h + 1) % D->size;
  }
  if (!D->table[h]) {
    D->table[h] = malloc(sizeof(node));
    memset(D->table[h], 0, sizeof(node));
    D->table[h]->user = name;
    D->table[h]->hosts = malloc(sizeof(host));
    D->table[h]->hosts->host_ = hostname;
    D->table[h]->hosts->port = port;
    D->table[h]->hosts->next = NULL;
    D->num_elem++;
  } else if (!strcmp(D->table[h]->user, name)) {
    new_host = malloc(sizeof(host));
    new_host->host_ = hostname;
    new_host->port = port;
    new_host->next = D->table[h]->hosts;
    D->table[h]->hosts = new_host;
  }
}

void insertNode(node *item, dict_t *D) {
  int h;
  h = hash(item->user,D->size);
  while (D->table[h] && strcmp(D->table[h]->user,item->user)) {
    h = (h + 1) % D->size;
  }
  if (!D->table[h]) {
    D->table[h] = item;
    D->num_elem++;
  }
}

void resize(dict_t *D) {
  node **table, **swap;
  int size, i;
  size = D->size*2;
  swap = (node **)malloc(size*sizeof(node *));
  memset(swap,  0, size*sizeof(node *));

  table = D->table;
  D->table = swap;
  D->num_elem = 0;
  i = size;
  size = D->size;
  D->size = i;
  
  for(i=0; i<size; i++) {
    if(table[i] && table[i]->state > -1){
      insertNode(table[i], D);
    }
  }
  free(table);
  //outputD(D);
}

dict_t *newD(int initial_size) {
  dict_t *D;
  int i;
  int size;
  D = (dict_t *)malloc(sizeof(dict_t));
  size = nextPrime(initial_size*2);
  D->size = size;
  D->table = (node **)malloc(size * sizeof(node *));
  memset(D->table, 0, size*sizeof(node *));
  D->num_elem = 0;
  return D;
}
 
void outputD(dict_t *D) {
  int i;
  boolean first;
  host *runner;

  for (i = 0; i < D->size; i++) {
    if (D->table[i] != NULL && D->table[i]->state > -1) {
      printf("user: ");
      printf("%s\n",D->table[i]->user);
      runner = D->table[i]->hosts;
      while(runner) {
	printf("\t%s, %s\n", runner->host_, runner->port);
	runner = runner->next;
      }
    }
  }
}

