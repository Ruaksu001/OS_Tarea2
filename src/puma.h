// puma.h define la estructura y funciones del hilo del puma
#ifndef PUMA_H
#define PUMA_H

#include "mesa.h"
#include <pthread.h>

typedef struct {
    Mesa *mesa;           // puntero a la mesa donde juega
    unsigned int semilla; // semilla local para aislar aleatoriedad
    pthread_t thread;
    int creado;           // 1 si el thread se creo bien, 0 si no
} Puma;

void *rutina_puma(void *arg);

// inicializa y ejecuta el thread del puma
void puma_iniciar(Puma *puma, Mesa *m);

// esperar a que termine el hilo del puma
void puma_esperar(Puma *puma);

#endif