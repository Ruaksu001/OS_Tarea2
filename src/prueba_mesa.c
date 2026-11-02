#include "mesa.h"
#include "perro.h"
#include "puma.h"
#include "tablero.h"
#include <stdio.h>
#include <stdlib.h>

// funcion que revisa que no haya corrupcion de memoria ni race conditions
static void verificar_invariantes(Mesa *m) {
    int errores = 0;

    // 1. el puma debe estar donde dice estar y ser un PUMA
    int pos_puma = m->tablero.pos_ficha[ID_PUMA];
    if (m->tablero.casilla[pos_puma] != PUMA) {
        printf("error: la casilla %d no tiene al puma\n", pos_puma);
        errores++;
    }

    // 2. contar perros vivos segun el arreglo de posiciones
    int perros_vivos = 0;
    for (int i = 1; i <= NUM_PERROS; i++) {
        int pos = m->tablero.pos_ficha[i];
        if (pos != CAPTURADO) {
            perros_vivos++;
            if (m->tablero.casilla[pos] != PERRO) {
                printf("error: la casilla %d no tiene al perro %d\n", pos, i);
                errores++;
            }
        }
    }

    // 3. contar casillas que dicen PERRO en el tablero fisico
    int casillas_perro = 0;
    for (int i = 0; i < NUM_NODOS; i++) {
        if (m->tablero.casilla[i] == PERRO) {
            casillas_perro++;
        }
    }

    if (casillas_perro != (NUM_PERROS - m->tablero.capturas)) {
        printf("error: hay %d casillas con perro, pero deberian ser %d\n", casillas_perro,
               NUM_PERROS - m->tablero.capturas);
        errores++;
    }

    if (errores == 0) {
        printf("invariantes ok\n");
    } else {
        printf("fallaron los invariantes\n");
    }
}

int main(int argc, char *argv[]) {
    // semilla por defecto 42, o la que se pase por consola
    unsigned int semilla = 42;
    if (argc > 1) {
        semilla = (unsigned int)atoi(argv[1]);
    }

    printf("iniciando prueba con semilla %u\n", semilla);

    grafo_iniciar();

    Mesa m;
    // id 1, retardo corto (10-30ms) para que termine rapido, max 2000 movimientos, imprime cada 50
    mesa_iniciar(&m, 1, 10, 30, semilla, 2000, 50, stdout);

    Puma puma;
    Perro perros[NUM_PERROS];

    // se lanzan todos los hilos
    puma_iniciar(&puma, &m);
    perros_iniciar(perros, &m);

    // se espera a que la partida termine
    puma_esperar(&puma);
    perros_esperar(perros);

    mesa_destruir(&m);

    // verificar que la concurrencia no rompio el tablero
    verificar_invariantes(&m);

    return 0;
}