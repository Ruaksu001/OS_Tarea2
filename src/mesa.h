// mesa.h define la estructura de una mesa de juego (tablero, mutex y estado de la partida)
// y las funciones con las que las fichas piden moverse
// todo el acceso al tablero pasa por mesa.c, asi los hilos de los perros y del puma
// nunca manejan el mutex directamente

#ifndef MESA_H
#define MESA_H

#include "tablero.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>

#define TURNO_MOVIO 1          // la ficha se movio
#define TURNO_SIN_MOVIMIENTO 0 // la ficha no tenia a donde moverse, lo vuelve a intentar despues
#define TURNO_TERMINAR -1      // la partida termino o el perro fue capturado, el hilo debe salir
#define EMPATE 3               // sigue a GANA_PUMA (1) y GANA_PERROS (2) de tablero.h

#define MAX_MOVIMIENTOS_DEFECTO 1000 // tope de movimientos si el parametro no es valido
#define IMPRIMIR_CADA_DEFECTO 50     // cada cuantos movimientos se imprime el tablero si el parametro no es valido

#define GANA_PUMA_ABANDONO 4   // los perros dejaron de responder
#define GANA_PERROS_ABANDONO 5 // el puma dejo de responder
#define CANCELADA 6            // el servidor cancelo la partida (Ctrl+C)

typedef struct {
    int id;                // numero de la mesa
    Tablero tablero;       // tablero de esta partida
    pthread_mutex_t mutex; // protege el tablero y los contadores de la mesa
    // atomic_int para que los hilos vean siempre el valor real de la bandera aunque la lean sin el mutex
    atomic_int partida_activa;
    int resultado;        // SIN_GANADOR, GANA_PUMA, GANA_PERROS, EMPATE, abandono o CANCELADA
    int movimientos;      // movimientos hechos hasta ahora
    int max_movimientos;  // tope de movimientos, al llegar a el la partida es empate
    int imprimir_cada;    // cada cuantos movimientos se imprime el tablero
    int delay_min;        // retardo minimo entre movimientos de una ficha (ms)
    int delay_max;        // retardo maximo entre movimientos de una ficha (ms)
    unsigned int semilla; // semilla base de la mesa, cada ficha usa esta mas su id
    FILE *salida;         // donde se escribe el log (pantalla o archivo)

    long long latido_puma_ms;   // ultimo momento (ms) en que el puma intento moverse
    long long latido_perros_ms; // ultimo momento (ms) en que un perro intento moverse
    atomic_int puma_caido;      // 1 si el jugador puma fallo (abandono simulado), sus fichas dejan de moverse
    atomic_int perros_caido;    // 1 si el jugador perros fallo (abandono simulado)
} Mesa;

// prototipos
void mesa_iniciar(Mesa *m, int id, int delay_min, int delay_max, unsigned int semilla, int max_movimientos,
                  int imprimir_cada, FILE *salida);
void mesa_destruir(Mesa *m);
int mesa_mover_perro(Mesa *m, int id_perro, unsigned int *semilla);
int mesa_mover_puma(Mesa *m, unsigned int *semilla);
const char *mesa_nombre_resultado(int resultado);

// duerme ms milisegundos en tramos cortos y sale antes si la partida termina
void mesa_dormir(Mesa *m, int ms);

// el juez llama a esto para ver si un bando lleva mas de limite_ms sin moverse y declarar abandono
void mesa_revisar_abandono(Mesa *m, int limite_ms);

// simula la falla de un jugador, sus fichas dejan de moverse sin avisar al juez
void mesa_marcar_caido(Mesa *m, int es_puma);

// cancela la partida por orden del servidor, si ya habia terminado no hace nada
void mesa_cancelar(Mesa *m);

#endif