// tablero.c se encarga del tablero del komikan
// construye el grafo, cada nodo guarda la lista de nodos con los que esta
// conectado las casillas del 5x5 se conectan con la de la derecha y la de abajo y el triangulo se conecta a mano al final de grafo_iniciar()
// como el grafo solo se escribe una vez al inicio del programa despues los threads solo lo leen

#include <stdio.h>
#include <pthread.h>
#include "tablero.h"

typedef enum { // declaracion de los estados de una casilla
INVALIDO = -1,
VACIO = 0,
PUMA = 1,
PERRO = 2
} EstadoCasilla;



void generar_tablero() {


}

static int vecinos[NUM_NODOS][MAX_VECINOS]; // lista de vecinos de cada nodo
static int cantidad[NUM_NODOS];             // cuantos vecinos tiene cada nodo

// funcion que conecta dos nodos en ambos sentidos
static void agregar_arista(int a, int b) {
    vecinos[a][cantidad[a]++] = b;
    vecinos[b][cantidad[b]++] = a;
}

// funcion que construye todo el grafo del tablero
void grafo_iniciar(void) {
    // partir con todos los nodos sin vecinos
    for (int n = 0; n < NUM_NODOS; n++) {
        cantidad[n] = 0;
    }

    // conectar las casillas del 5x5 cada una con la de su derecha y la de abajo
    for (int f = 0; f < FILAS; f++) {
        for (int c = 0; c < COLUMNAS; c++) {
            int n = f * COLUMNAS + c; // numero de la casilla en la fila f columna c

            if (c + 1 < COLUMNAS) {
                agregar_arista(n, n + 1);
            }

            if (f + 1 < FILAS) {
                agregar_arista(n, n + COLUMNAS);
            }
        }
    }

    // conexiones del triangulo base entre los dos nodos intermedios
    agregar_arista(NODO_IZQ, NODO_DER);

    // los dos nodos intermedios con el trono (en diagonal)
    agregar_arista(NODO_IZQ, NODO_TRONO);
    agregar_arista(NODO_DER, NODO_TRONO);

    // nodo intermedio izquierdo esquina (diagonal) y casilla de abajo
    agregar_arista(NODO_IZQ, 0);
    agregar_arista(NODO_IZQ, 1);

    // nodo intermedio derecho casilla de abajo y esquina (diagonal)
    agregar_arista(NODO_DER, 3);
    agregar_arista(NODO_DER, 4);

    // el trono tambien baja directo a la casilla central de la fila de arriba
    agregar_arista(NODO_TRONO, 2);
}

// funcion que retorna cuantos vecinos tiene un nodo
int cantidad_vecinos(int nodo) {
    return cantidad[nodo];
}

// funcion que retorna el vecino numero i de un nodo
int vecino_de(int nodo, int i) {
    return vecinos[nodo][i];
}

// funcion que revisa si b esta en la lista de vecinos de a
int son_vecinos(int a, int b) {
    for (int i = 0; i < cantidad[a]; i++) {
        if (vecinos[a][i] == b) {
            return 1;
        }
    }

    return 0;
}