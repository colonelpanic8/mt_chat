#ifndef _DICT_H
#define _DICT_H

#include "prelude.h"

dict_t *newD(int initial_size);
boolean containsD(char *name, dict_t *D);
void insertD(char *name, dict_t *D);
void outputD(dict_t *D);

#endif
