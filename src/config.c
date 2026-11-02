#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

void inicializar_config(Config *c) {
    c->n_jugadores = 0; // numero de jugadores por defecto = 0
    c->m_mesas = 1; // numero de mesas por defecto = 1
    c->delay_min = 50; // retardo minimo por defecto = 50ms
    c->delay_max = 300; // retardo maximo por defecto = 300ms
    c->seed = (unsigned int)time(NULL); // la seed se genera en base al tiempo
}