#ifndef PERRO_H
#define PERRO_H

#include <pthread.h>
#include "tablero.h"

typedef struct {
    int id;                       // id del perro
    Tablero *tablero;             // puntero al tablero
} Perro;

// llena destinos_validos con las casillas vacias adyacentes al perro
// retorna la cantidad de opciones libres encontradas
int obtener_movimientos_perro(const Tablero *t, int id_perro, int destinos_validos[MAX_VECINOS]);

// ejecuta el movimiento en el tablero si la casilla sigue libre
// retorna 1 si tuvo exito y 0 en caso contrario
int mover_perro(Tablero *t, int id_perro, int destino);

#endif

