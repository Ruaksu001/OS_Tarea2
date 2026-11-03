// perro.c implementa la rutina y logica de movimiento de los perros delegando en la mesa
#define _DEFAULT_SOURCE
#include "perro.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// funcion que retorna la cantidad de movimientos posibles de un perro
// llena el arreglo destinos_validos con los nodos a los que el perro se puede mover
int obtener_movimientos_perro(const Tablero *t, int id_perro, int destinos_validos[MAX_VECINOS]) {
    // verificar que el perro es valido
    if (id_perro < 1 || id_perro > NUM_PERROS) {
        return 0;
    }

    int origen = t->pos_ficha[id_perro];
    if (origen == CAPTURADO) {
        return 0;
    }

    int total = 0;
    // saca numero de vecinos segun valor de adyacencia de la posicion de origen
    int n_vecinos = cantidad_vecinos(origen);

    for (int i = 0; i < n_vecinos; i++) {
        // se busca la id del vecino numero i desde el origen para buscarlo en el tablero
        int vecino = vecino_de(origen, i);
        if (t->casilla[vecino] == VACIO) {
            destinos_validos[total] = vecino; // guarda casilla libre en la lista
            total++;                          // aumenta la cantidad de vecinos libres
        }
    }

    // retorna total de vecinos libres
    return total;
}

// funcion que mueve al perro hacia una casilla entregada como parametro (destino)
int mover_perro(Tablero *t, int id_perro, int destino) {
    if (id_perro < 1 || id_perro > NUM_PERROS) {
        return 0;
    }

    int origen = t->pos_ficha[id_perro];
    if (origen == CAPTURADO) {
        return 0;
    }

    // verificar que el destino sea adyacente y este libre
    if (!son_vecinos(origen, destino) || t->casilla[destino] != VACIO) {
        return 0;
    }

    // actualizar el estado del tablero y posicion de la ficha
    t->casilla[origen] = VACIO;
    t->casilla[destino] = PERRO;
    t->pos_ficha[id_perro] = destino;

    // retorna 1 si tiene exito
    return 1;
}

// rutina que cada perro ejecuta continuamente de forma paralela
void *rutina_perro(void *arg) {
    Perro *p = arg;
    unsigned int seed = p->semilla;

    while (p->mesa->partida_activa) {
        // esperar entre el delay minimo y maximo definido en la mesa
        int rango = p->mesa->delay_max - p->mesa->delay_min + 1;
        int tiempo_ms = p->mesa->delay_min + (rand_r(&seed) % rango);
        mesa_dormir(p->mesa, tiempo_ms);

        // le pide a la mesa que lo mueva, si retorna terminar sale del ciclo
        int estado = mesa_mover_perro(p->mesa, p->id, &seed);
        if (estado == TURNO_TERMINAR) {
            break;
        }
    }

    return NULL;
}

// inicializa los perros y crea threads correspondientes
void perros_iniciar(Perro perros[NUM_PERROS], Mesa *m) {
    for (int i = 0; i < NUM_PERROS; i++) {
        perros[i].id = i + 1;
        perros[i].mesa = m;
        perros[i].semilla = m->semilla + perros[i].id;
        perros[i].creado = 0;

        if (pthread_create(&perros[i].thread, NULL, rutina_perro, &perros[i]) == 0) {
            perros[i].creado = 1;
        } else {
            fprintf(stderr, "error al crear el hilo del perro %d\n", perros[i].id);
        }
    }
}

// espera a que los threads creados terminen
void perros_esperar(Perro perros[NUM_PERROS]) {
    for (int i = 0; i < NUM_PERROS; i++) {
        // solo hace join si el thread fue creado exitosamente
        if (perros[i].creado) {
            pthread_join(perros[i].thread, NULL);
        }
    }
}