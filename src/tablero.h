// tablero.h establece las constantes del tablero del komikan y las funciionalidades del grafo de adyacencias
// para saber que casillas estan enlazadas entre si
// el tablero lo modelamos como un grafo de 28 nodos los primeros 25 son la cuadricula 5x5
// van por fila del 0 al 24 y los ultimos 3 arman el triangulo de arriba q seria el trono

#ifndef TABLERO_H // comprobar si no esta definido, caso contrario salta a #endif
#define TABLERO_H // definirlo

#include <stdio.h>

#define FILAS 5        // filas de la cuadricula main
#define COLUMNAS 5     //columnas de la misma cuadricula
#define NUM_NODOS 28   // 25 casillas mas los 2 nodos intermedios y el mismisimo trono
#define NODO_IZQ 25    //el nodo que va a la izq del triangulo
#define NODO_DER 26    //el nodo que va a la derecha
#define NODO_TRONO 27  // la punta del triangulo rey
#define MAX_VECINOS 4  //maximo limite de conexiones para que un nodo no colapse
#define NUM_PERROS 12  // cantidad de perros por partida
#define NUM_FICHAS 13  // el puma mas los 12 perros
#define ID_PUMA 0      // el puma tiene id 0, los perros van del 1 al 12
#define CAPTURADO -1   // posicion de un perro que ya fue capturado

typedef enum { // declaracion de los estados de una casilla
INVALIDO = -1,
VACIO = 0,
PUMA = 1,
PERRO = 2
} EstadoCasilla;

// estado completo de un tablero
typedef struct {
    EstadoCasilla casilla[NUM_NODOS]; // que hay en cada nodo
    int pos_ficha[NUM_FICHAS];        // en que nodo esta cada ficha
    int capturas;                     // perros capturados hasta ahora
} Tablero;

//funcion que arma la lista de vecinos para cada nodo se hace run una sola vez antes de lanzaar los hilos
void grafo_iniciar(void);

//retorna la cantidad de vecinos que tiene un nodo
int cantidad_vecinos(int nodo);

// retorna el vecino num i de un nodo
int vecino_de(int nodo, int i);

//  ver si existe conexion entre nodos a y b
int son_vecinos(int a, int b);

// deja el tablero con el puma en el trono y los 12 perros en su posicion inicial
void generar_tablero(Tablero *t);

// retorna que hay en un nodo, o INVALIDO si el nodo no existe
EstadoCasilla estado_de(const Tablero *t, int nodo);

// imprime el tablero en la salida indicada (pantalla o archivo)
void imprimir_tablero(const Tablero *t, FILE *salida);

#endif