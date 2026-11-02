// puma.c implementa la rutina del puma delegando en la mesa
#define _DEFAULT_SOURCE
#include "puma.h"
#include "tablero.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void *rutina_puma(void *arg) {
    Puma *p = arg;
    unsigned int seed = p->semilla;

    while (p->mesa->partida_activa) {
        // esperar el rango definido con variables de delay de la mesa
        int rango = p->mesa->delay_max - p->mesa->delay_min + 1;
        int tiempo = p->mesa->delay_min + (rand_r(&seed) % rango);
        usleep(tiempo * 1000);

        // le pide a la mesa que lo mueva, si retorna terminar sale del ciclo
        int estado = mesa_mover_puma(p->mesa, &seed);
        if (estado == TURNO_TERMINAR) {
            break;
        }
    }
    
    return NULL;
}

// inicializa y ejecuta el thread del puma
void puma_iniciar(Puma *puma, Mesa *m) {
    puma->mesa = m;
    puma->semilla = m->semilla + ID_PUMA; // la id del puma siempre es 0
    puma->creado = 0;

    if (pthread_create(&puma->thread, NULL, rutina_puma, puma) == 0) {
        puma->creado = 1;
    } else {
        fprintf(stderr, "error al crear el hilo del puma\n");
    }
}

// esperar a que termine el hilo del puma
void puma_esperar(Puma *puma) { 
    // solo hace join si el thread fue creado exitosamente
    if (puma->creado) {
        pthread_join(puma->thread, NULL); 
    }
}