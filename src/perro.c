#include <stdio.h>
#include "perro.h"

// funcion que retorna la cantidad de movimientos posibles de un perro
// llena el arreglo destinos_validos con los nodos a los que el perro se puede mover
int obtener_movimientos_perro(const Tablero *t, int id_perro, int destinos_validos[MAX_VECINOS]) {

    // verificar que el perro es valido
    if (id_perro < 1 || id_perro > NUM_PERROS) {
        return 0;
    }

    int origen = t->pos_ficha[id_perro];
    if (origen == CAPTURADO) return 0;


    int total = 0;
    int n_vecinos = cantidad[origen]; // saca numero de vecinos segun valor de adyacencia de la posicion de origen

    for (int i = 0; i < n_vecinos; i++) {
        // se busca la id del vecino numero i desde el origen para buscarlo en el tablero
        int vecino = vecinos[origen][i];
        if (t->casilla[vecino] == VACIO) {
            destinos_validos[total] = vecino; // guarda casilla libre en la lista
            total++; // aumenta la cantidad de vecinos libres
        }
    }

    // retorna total de vecinos libres
    return total;
    
}

// funcion que mueve al perro hacia una casilla entregada como parametro (destino)
int mover_perro(Tablero *t, int id_perro, int destino) {
    if (id_perro < 1 || id_perro > NUM_PERROS) return 0;

    int origen = t->pos_ficha[id_perro];
    if (origen == CAPTURADO) return 0;

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
