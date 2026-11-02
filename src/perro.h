// perro.h define la estructura y funciones del hilo de cada perro
#ifndef PERRO_H
#define PERRO_H

#include "mesa.h"
#include <pthread.h>

typedef struct {
    int id;               // id del perro
    Mesa *mesa;           // puntero a la mesa donde juega
    unsigned int semilla; // semilla local para aislar aleatoriedad
    pthread_t thread;     // id del thread
    int creado;           // 1 si el thread se creo bien, 0 si no
} Perro;

// llena destinos_validos con las casillas vacias adyacentes al perro
// retorna la cantidad de opciones libres encontradas
int obtener_movimientos_perro(const Tablero *t, int id_perro, int destinos_validos[MAX_VECINOS]);

// ejecuta el movimiento en el tablero si la casilla sigue libre
// retorna 1 si tuvo exito y 0 en caso contrario
int mover_perro(Tablero *t, int id_perro, int destino);

void *rutina_perro(void *arg);

// funcion que inicializa los perros y ejecuta un thread para cada uno
void perros_iniciar(Perro perros[NUM_PERROS], Mesa *m);

// funcion que espera a que los 12 threads terminen
void perros_esperar(Perro perros[NUM_PERROS]);

#endif