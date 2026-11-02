// en config.h se almacenan los argumentos de ejecucion del programa y el archivo config.txt

#ifndef CONFIG_H
#define CONFIG_H

#include "tablero.h"
#include <stdio.h>

// struct con parametros de ejecucion
typedef struct {
    // argumentos obligatorios
    int n_jugadores; // N: numero maximo de clientes/jugadores a procesar
    int m_mesas; // M: maximo de mesas concurrentes

    // parametros de config.txt
    int delay_min; // retardo minimo por movimiento en ms
    int delay_max; // retardo maximo por movimiento en ms
    unsigned seed; // semilla aleatoria para reproducibilidad de pruebas
} Config;

// inicializar configuracion con valores por defecto
void inicializar_config(Config *c);

// parsear el config.txt
static int parse_config(Config *c, const char *ruta);

//
int cargar_config(Config *c, int argc, char *argv[]);

#endif