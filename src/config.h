// en config.h se almacenan los argumentos de ejecucion del programa y el
// archivo config.txt para ello utilizar un struct Config

#ifndef CONFIG_H
#define CONFIG_H

#include "tablero.h"
#include <stdio.h>

// struct con parametros de ejecucion
typedef struct {
    // argumentos obligatorios
    int n_jugadores; // N: numero maximo de clientes/jugadores a procesar
    int m_mesas;     // M: maximo de mesas concurrentes

    // parametros de config.txt
    int delay_min; // retardo minimo por movimiento en ms
    int delay_max; // retardo maximo por movimiento en ms
    unsigned seed; // semilla aleatoria para reproducibilidad de pruebas
} Config;

// inicializar configuracion con valores por defecto
void inicializar_config(Config *c);

// utilizado para cargar config.txt si es que existe y recibir parametros N y M
// las configuraciones estan en el struct Config entregado como parametro
int cargar_config(Config *c, int argc, char *argv[]);

#endif