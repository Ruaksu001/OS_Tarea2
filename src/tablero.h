// tablero.h establece las constantes del tablero del komikan y las funciionalidades del grafo de adyacencias
// para saber que casillas estan enlazadas entre si
// el tablero lo modelamos como un grafo de 28 nodos los primeros 25 son la cuadricula 5x5
// van por fila del 0 al 24 y los ultimos 3 arman el triangulo de arriba q seria el trono

#ifndef TABLERO_H // comprobar si no esta definido, caso contrario salta a #endif
#define TABLERO_H // definirlo

#define FILAS 5        // filas de la cuadricula main
#define COLUMNAS 5     //columnas de la misma cuadricula
#define NUM_NODOS 28   // 25 casillas mas los 2 nodos intermedios y el mismisimo trono
#define NODO_IZQ 25    //el nodo que va a la izq del triangulo
#define NODO_DER 26    //el nodo que va a la derecha
#define NODO_TRONO 27  // la punta del triangulo rey
#define MAX_VECINOS 4  //maximo limite de conexiones para que un nodo no colapse

//funcion que arma la lista de vecinos para cada nodo se hace run una sola vez antes de lanzaar los hilos
void grafo_iniciar(void);

//retorna la cantidad de vecinos que tiene un nodo
int cantidad_vecinos(int nodo);

// retorna el vecino num i de un nodo
int vecino_de(int nodo, int i);

//  ver si existe conexion entre nodos a y b
int son_vecinos(int a, int b);

#endif