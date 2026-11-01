#ifndef PERRO_H
#define PERRO_H

#include "tablero.h"
#include <pthread.h>

typedef struct {
  int id;           // id del perro
  Tablero *tablero; // puntero al tablero
  pthread_mutex_t *mutex;
  volatile int *partida_activa; // flag que indica si el juego sigue en curso
  int delay_min;                // delay minimo de ciclo de decision
  int delay_max;                // delay maximo de ciclo de decision
  pthread_t thread;             // id del thread
} Perro;

// llena destinos_validos con las casillas vacias adyacentes al perro
// retorna la cantidad de opciones libres encontradas
int obtener_movimientos_perro(const Tablero *t, int id_perro,
                              int destinos_validos[MAX_VECINOS]);

// ejecuta el movimiento en el tablero si la casilla sigue libre
// retorna 1 si tuvo exito y 0 en caso contrario
int mover_perro(Tablero *t, int id_perro, int destino);

void *rutina_perro(void *arg);

// funcion que inicializa los perros y ejecuta un thread para cada uno
void perros_iniciar(Perro perros[NUM_PERROS], Tablero *t,
                    pthread_mutex_t *mutex, volatile int *partida_activa,
                    int delay_min, int delay_max);

// funcion que espera a que los 12 threads terminen
void perros_esperar(Perro perros[NUM_PERROS]);

#endif
