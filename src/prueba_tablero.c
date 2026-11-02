// prueba_tablero.c prueba el tablero a medida que se agregan funciones, no es parte del torneo final
// por ahora imprime la lista de vecinos de cada nodo y el tablero inicial

#include "tablero.h"
#include <stdio.h>

// deja el tablero sin fichas, con todos los perros capturados (solo para armar escenarios de prueba)
static void vaciar_tablero(Tablero *t) {
    for (int n = 0; n < NUM_NODOS; n++) {
        t->casilla[n] = VACIO;
    }
    for (int id = 0; id < NUM_FICHAS; id++) {
        t->pos_ficha[id] = CAPTURADO;
    }
    t->capturas = 0;
}

// pone una ficha en un nodo sin pasar por las reglas de movimiento (solo para armar escenarios de prueba)
static void poner_ficha(Tablero *t, int id, int nodo) {
    if (t->pos_ficha[id] != CAPTURADO) {
        t->casilla[t->pos_ficha[id]] = VACIO;
    }
    t->pos_ficha[id] = nodo;
    t->casilla[nodo] = (id == ID_PUMA) ? PUMA : PERRO;
}

int main(void) {
    Tablero t;

    grafo_iniciar();
    generar_tablero(&t);

    // imprimir los vecinos de cada nodo para comprobar las conexiones
    for (int n = 0; n < NUM_NODOS; n++) {
        printf("nodo %2d:", n);

        for (int i = 0; i < cantidad_vecinos(n); i++) {
            printf(" %2d", vecino_de(n, i));
        }

        printf("\n");
    }

    // imprimir el tablero inicial y contar los perros para comprobar la colocacion
    printf("\n");
    imprimir_tablero(&t, stdout);

    int perros = 0;
    for (int id = 1; id <= NUM_PERROS; id++) {
        if (t.pos_ficha[id] != CAPTURADO) {
            perros++;
        }
    }

    printf("perros en el tablero: %d\n", perros);
    // imprimir los saltos del puma desde cada nodo (casilla que salta -> casilla donde cae)
    printf("\n");
    int total_saltos = 0;
    for (int n = 0; n < NUM_NODOS; n++) {
        printf("saltos desde %2d:", n);
        for (int i = 0; i < cantidad_saltos(n); i++) {
            printf(" %2d->%2d", salto_medio(n, i), salto_destino(n, i));
            total_saltos++;
        }
        printf("\n");
    }
    printf("total de saltos: %d\n", total_saltos);

    // prueba de movimientos del puma: dos pasos y despues un salto que captura al perro 1
    printf("\n");
    printf("paso 27 -> 2: %d (esperado 1)\n", mover_puma(&t, 2));
    printf("paso 2 -> 7: %d (esperado 1)\n", mover_puma(&t, 7));
    poner_ficha(&t, 1, 12);
    poner_ficha(&t, 3, 13);
    imprimir_tablero(&t, stdout);

    printf("salto 7 -> 17: %d (esperado 2)\n", mover_puma(&t, 17));
    printf("capturas: %d, perro 1 capturado: %d\n", t.capturas, t.pos_ficha[1] == CAPTURADO);
    printf("movimiento invalido 17 -> 27: %d (esperado 0)\n", mover_puma(&t, 27));
    imprimir_tablero(&t, stdout);

    // prueba de victoria de los perros: puma en la esquina 24 rodeado por los perros 1 al 4
    vaciar_tablero(&t);
    poner_ficha(&t, ID_PUMA, 24);
    poner_ficha(&t, 1, 19);
    poner_ficha(&t, 2, 23);
    poner_ficha(&t, 3, 14);
    poner_ficha(&t, 4, 22);

    printf("\n");
    imprimir_tablero(&t, stdout);
    printf("acorralado: %d, ganador: %d (esperado 1 y 2)\n", puma_acorralado(&t), tablero_ganador(&t));

    // si el perro 3 se va de la casilla 14 el puma puede saltar sobre el perro 1
    poner_ficha(&t, 3, 9);
    printf("acorralado: %d, ganador: %d (esperado 0 y 0)\n", puma_acorralado(&t), tablero_ganador(&t));

    // prueba de victoria del puma: 6 capturas
    t.capturas = CAPTURAS_PARA_GANAR;
    printf("ganador con 6 capturas: %d (esperado 1)\n", tablero_ganador(&t));

    return 0;
}