// prueba_tablero.c prueba el tablero a medida que se agregan funciones, no es parte del torneo final
// por ahora imprime la lista de vecinos de cada nodo y el tablero inicial

#include <stdio.h>
#include "tablero.h"

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

    return 0;
}