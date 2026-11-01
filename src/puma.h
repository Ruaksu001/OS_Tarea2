#ifndef PUMA_H
#define PUMA_H

#include "tablero.h"
#include <pthread.h>

/*
NOTA: se utiliza volatile int para desactivar optimizaciones del compilador y
que la variable partida_activa siempre sea accesible, en caso contrario, el
compilador asumiria logica erronea
*/

typedef struct {
  int id;                       // id del puma (siempre 0)
  Tablero *tablero;             // puntero al tablero
  pthread_mutex_t *mutex;       // mutex del puma
  volatile int *partida_activa; // flag de estado de la partida
  int delay_min;
  int delay_max;
  pthread_t thread;
} Puma;

#endif
