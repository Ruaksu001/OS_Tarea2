// tablero.c se encarga del tablero del komikan
// construye el grafo, cada nodo guarda la lista de nodos con los que esta
// conectado las casillas del 5x5 se conectan con la de la derecha y la de abajo y el triangulo se conecta a mano al final de grafo_iniciar()
// como el grafo solo se escribe una vez al inicio del programa despues los threads solo lo leen
// tambien arma el estado inicial del tablero (puma y perros) y lo imprime

#include <stdio.h>
#include <pthread.h>
#include "tablero.h"

// funcion que pone una ficha en un nodo, el id 0 es el puma y del 1 al 12 son los perros
static void colocar_ficha(Tablero *t, int id, int nodo) {
    t->pos_ficha[id] = nodo;

    if (id == ID_PUMA) {
        t->casilla[nodo] = PUMA;
    } else {
        t->casilla[nodo] = PERRO;
    }
}

// funcion que deja el tablero listo para empezar una partida
void generar_tablero(Tablero *t) {
    // partir con todas las casillas vacias
    for (int n = 0; n < NUM_NODOS; n++) {
        t->casilla[n] = VACIO;
    }

    t->capturas = 0;

    // el puma parte en el trono
    colocar_ficha(t, ID_PUMA, NODO_TRONO);

    // perros del 1 al 10 en las dos filas de abajo, casillas 15 a 24
    for (int id = 1; id <= 10; id++) {
        colocar_ficha(t, id, 14 + id);
    }

    // perros 11 y 12 en los extremos de la fila del medio
    colocar_ficha(t, 11, 10);
    colocar_ficha(t, 12, 14);
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

    // los dos nodos intermedios con el trono en diagonal
    agregar_arista(NODO_IZQ, NODO_TRONO);
    agregar_arista(NODO_DER, NODO_TRONO);

    // nodo intermedio izquierdo esquina diagonal y casilla de abajo
    agregar_arista(NODO_IZQ, 0);
    agregar_arista(NODO_IZQ, 1);

    // nodo intermedio derecho casilla de abajo y esquina diagonal
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

// funcion que retorna que hay en un nodo
EstadoCasilla estado_de(const Tablero *t, int nodo) {
    if (nodo < 0 || nodo >= NUM_NODOS) {
        return INVALIDO;
    }

    return t->casilla[nodo];
}

// funcion que convierte el estado de una casilla en el caracter que se imprime
static char simbolo(EstadoCasilla estado) {
    if (estado == PUMA) {
        return 'P';
    }

    if (estado == PERRO) {
        return 'o';
    }

    return '.';
}

// funcion que imprime el tablero, primero el triangulo y despues la cuadricula
void imprimir_tablero(const Tablero *t, FILE *salida) {
    fprintf(salida, "        %c\n", simbolo(t->casilla[NODO_TRONO]));
    fprintf(salida, "    %c       %c\n", simbolo(t->casilla[NODO_IZQ]), simbolo(t->casilla[NODO_DER]));

    for (int f = 0; f < FILAS; f++) {
        for (int c = 0; c < COLUMNAS; c++) {
            fprintf(salida, "%c", simbolo(t->casilla[f * COLUMNAS + c]));

            if (c + 1 < COLUMNAS) {
                fprintf(salida, "   ");
            }
        }

        fprintf(salida, "\n");
    }
}