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
  unsigned int semilla;         // semilla local para aislar aleatoriedad
  pthread_t thread;
} Puma;

void *rutina_puma(void *arg);

// inicializa y ejecuta el thread del puma
void puma_iniciar(Puma *puma, Tablero *t, pthread_mutex_t *mutex,
                  volatile int *partida_activa, int delay_min, int delay_max,
                  unsigned int semilla_base);

// esperar a que termine el hilo del puma
void puma_esperar(Puma *puma);

#endif