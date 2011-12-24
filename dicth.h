#ifndef _DICTH_H
#define _DICTH_H

// hash table implementation of a dictionary
typedef struct _host {
  char *host_;
  char *port;
  struct _host *next;
} host;

typedef struct _node {
  int state; //-1 for dirty, 0 for empty, 1 for full
  char *user;
  host *hosts;
} node;

typedef struct _dict_t {
  int num_elem;
  int size;
  node **table;
} dict_t;

typedef struct hostport_ {
  char port[10];
  char host[256];
} hostport;


int gethp(char *name, hostport **arr, dict_t *D);
node *get(char *name, dict_t *D);
dict_t *newD(int initial_size);
boolean containsD(char *name, dict_t *D);
void insertD(char *name, char *host, char *port, dict_t *D);
void outputD(dict_t *D);
void resize(dict_t *D);

#endif
