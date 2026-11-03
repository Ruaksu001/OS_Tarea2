// mesa.c se encarga de los movimientos de las fichas dentro de una mesa
// cada movimiento (elegir destino, mover, escribir el log y revisar si alguien gano) se hace
// completo con el mutex de la mesa tomado, asi solo una ficha a la vez puede cambiar el tablero
// el unico otro mutex es el del log, que es global y siempre se toma despues del de la mesa
// tambien tiene lo que el juez usa para vigilar la partida: abandono por inactividad y cancelacion

// define necesario para que compile rand_r, usleep y clock_gettime con -std=c17
#define _DEFAULT_SOURCE
#include "mesa.h"
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

// mutex global para el logeo, evita que dos mesas mezclen lineas en el archivo de salida
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

// funcion que retorna la hora actual en milisegundos con un reloj monotonico (no salta si cambian la hora del sistema)
static long long ahora_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

// funcion que retorna el texto asociado al resultado
const char *mesa_nombre_resultado(int resultado) {
    if (resultado == GANA_PUMA)
        return "gana el puma";
    if (resultado == GANA_PERROS)
        return "ganan los perros";
    if (resultado == EMPATE)
        return "empate";
    if (resultado == GANA_PUMA_ABANDONO)
        return "gana el puma (perros abandonaron)";
    if (resultado == GANA_PERROS_ABANDONO)
        return "ganan los perros (puma abandono)";
    if (resultado == CANCELADA)
        return "partida cancelada";
    return "sin ganador";
}

// inicializa la mesa guardando los parametros del config y preparando el mutex
void mesa_iniciar(Mesa *m, int id, int delay_min, int delay_max, unsigned int semilla, int max_movimientos,
                  int imprimir_cada, FILE *salida) {
    m->id = id;

    // si pasan los retardos invertidos, se arreglan solos
    if (delay_min > delay_max) {
        int temp = delay_min;
        delay_min = delay_max;
        delay_max = temp;
    }

    // un tope o una frecuencia de impresion menor o igual a 0 no tienen sentido, se usa el valor por defecto
    if (max_movimientos <= 0) {
        max_movimientos = MAX_MOVIMIENTOS_DEFECTO;
    }

    if (imprimir_cada <= 0) {
        imprimir_cada = IMPRIMIR_CADA_DEFECTO;
    }

    m->delay_min = delay_min;
    m->delay_max = delay_max;
    m->semilla = semilla;
    m->max_movimientos = max_movimientos;
    m->imprimir_cada = imprimir_cada;
    m->salida = salida;

    m->partida_activa = 1;
    m->resultado = SIN_GANADOR;
    m->movimientos = 0;

    m->puma_caido = 0;
    m->perros_caido = 0;

    pthread_mutex_init(&m->mutex, NULL);
    generar_tablero(&m->tablero);
    m->latido_puma_ms = ahora_ms();
    m->latido_perros_ms = ahora_ms();
}

// se llama solo despues de hacer join a todos los hilos
void mesa_destruir(Mesa *m) {
    pthread_mutex_destroy(&m->mutex);
}

// duerme en tramos de 20 ms para enterarse rapido si la partida termina, en vez de dormir todo el retardo de una vez
void mesa_dormir(Mesa *m, int ms) {
    while (ms > 0 && m->partida_activa) {
        int paso = ms > 20 ? 20 : ms;
        usleep(paso * 1000);
        ms -= paso;
    }
}

// funcion interna que escribe el log y revisa victoria (siempre se llama con el mutex de la mesa tomado)
static void registrar_movimiento(Mesa *m, const char *quien, int origen, int destino, int es_captura) {
    m->movimientos++;

    // orden fijo de locks: mesa primero, log despues (evita deadlocks)
    pthread_mutex_lock(&log_mutex);
    if (es_captura) {
        fprintf(m->salida, "[Mesa %d] mov %d: %s %d -> %d (captura, total %d)\n", m->id, m->movimientos, quien, origen,
                destino, m->tablero.capturas);
    } else {
        fprintf(m->salida, "[Mesa %d] mov %d: %s %d -> %d\n", m->id, m->movimientos, quien, origen, destino);
    }
    pthread_mutex_unlock(&log_mutex);

    // imprimir el tablero periodicamente si corresponde
    if (m->movimientos % m->imprimir_cada == 0) {
        pthread_mutex_lock(&log_mutex);
        fprintf(m->salida, "[Mesa %d] tablero en mov %d:\n", m->id, m->movimientos);
        imprimir_tablero(&m->tablero, m->salida);
        pthread_mutex_unlock(&log_mutex);
    }

    // evaluacion de victoria
    int ganador = tablero_ganador(&m->tablero);
    if (ganador != SIN_GANADOR) {
        m->resultado = ganador;
    } else if (m->movimientos >= m->max_movimientos) {
        m->resultado = EMPATE;
    }

    // si la partida termina, se saca la bandera y se imprime el resumen final
    if (m->resultado != SIN_GANADOR) {
        m->partida_activa = 0;

        pthread_mutex_lock(&log_mutex);
        fprintf(m->salida, "[Mesa %d] tablero final:\n", m->id);
        imprimir_tablero(&m->tablero, m->salida);
        fprintf(m->salida, "[Mesa %d] fin de la partida: %s (capturas: %d, movimientos: %d)\n", m->id,
                mesa_nombre_resultado(m->resultado), m->tablero.capturas, m->movimientos);
        pthread_mutex_unlock(&log_mutex);
    }
}

// encapsula la logica de pedir movimiento para un perro
int mesa_mover_perro(Mesa *m, int id_perro, unsigned int *semilla) {
    pthread_mutex_lock(&m->mutex);

    // si el jugador perros fallo, sus fichas ya no se mueven ni marcan latido
    if (m->perros_caido) {
        pthread_mutex_unlock(&m->mutex);
        return TURNO_TERMINAR;
    }

    m->latido_perros_ms = ahora_ms();

    // salir de inmediato si el juego termino o el perro ya fue comido
    if (!m->partida_activa || m->tablero.pos_ficha[id_perro] == CAPTURADO) {
        pthread_mutex_unlock(&m->mutex);
        return TURNO_TERMINAR;
    }

    int opciones[MAX_VECINOS];
    int cantidad = obtener_movimientos_perro(&m->tablero, id_perro, opciones);

    if (cantidad == 0) {
        pthread_mutex_unlock(&m->mutex);
        return TURNO_SIN_MOVIMIENTO;
    }

    int origen = m->tablero.pos_ficha[id_perro];
    int destino = opciones[rand_r(semilla) % cantidad];

    mover_perro(&m->tablero, id_perro, destino);

    char buffer[32];
    snprintf(buffer, sizeof(buffer), "perro %d", id_perro);
    registrar_movimiento(m, buffer, origen, destino, 0);

    pthread_mutex_unlock(&m->mutex);
    return TURNO_MOVIO;
}

// encapsula la logica de movimiento para el puma
int mesa_mover_puma(Mesa *m, unsigned int *semilla) {
    pthread_mutex_lock(&m->mutex);

    // si el jugador puma fallo, no se mueve ni marca latido
    if (m->puma_caido) {
        pthread_mutex_unlock(&m->mutex);
        return TURNO_TERMINAR;
    }

    m->latido_puma_ms = ahora_ms();

    if (!m->partida_activa) {
        pthread_mutex_unlock(&m->mutex);
        return TURNO_TERMINAR;
    }

    int origen = m->tablero.pos_ficha[ID_PUMA];
    int destino = -1;
    int opciones[MAX_SALTOS];
    int es_captura = 0;

    // el puma siempre prefiere capturar primero
    int cantidad = obtener_saltos_puma(&m->tablero, opciones);
    if (cantidad > 0) {
        destino = opciones[rand_r(semilla) % cantidad];
    } else {
        // si no hay capturas, intenta dar un paso
        cantidad = obtener_pasos_puma(&m->tablero, opciones);
        if (cantidad > 0) {
            destino = opciones[rand_r(semilla) % cantidad];
        }
    }

    if (destino == -1) {
        pthread_mutex_unlock(&m->mutex);
        return TURNO_SIN_MOVIMIENTO;
    }

    int res = mover_puma(&m->tablero, destino);
    if (res == MOV_CAPTURA) {
        es_captura = 1;
    }

    registrar_movimiento(m, "puma", origen, destino, es_captura);

    pthread_mutex_unlock(&m->mutex);
    return TURNO_MOVIO;
}

// el juez llama a esto para ver si un bando lleva mas de limite_ms sin moverse
void mesa_revisar_abandono(Mesa *m, int limite_ms) {
    pthread_mutex_lock(&m->mutex);

    // si el juego ya termino, no hace nada
    if (!m->partida_activa) {
        pthread_mutex_unlock(&m->mutex);
        return;
    }

    long long ahora = ahora_ms();
    int abandono = 0;

    if (ahora - m->latido_puma_ms >= limite_ms) {
        m->resultado = GANA_PERROS_ABANDONO;
        abandono = 1;
    } else if (ahora - m->latido_perros_ms >= limite_ms) {
        m->resultado = GANA_PUMA_ABANDONO;
        abandono = 1;
    }

    // si alguien supero el limite de tiempo, el juez termina la partida
    if (abandono) {
        m->partida_activa = 0;

        pthread_mutex_lock(&log_mutex);
        fprintf(m->salida, "[Mesa %d] fin por abandono: %s (sin actividad por mas de %d ms)\n", m->id,
                mesa_nombre_resultado(m->resultado), limite_ms);
        pthread_mutex_unlock(&log_mutex);
    }

    pthread_mutex_unlock(&m->mutex);
}

// simula la falla de un jugador, sus fichas dejan de moverse y no se avisa al juez (el juez lo detecta por los latidos)
void mesa_marcar_caido(Mesa *m, int es_puma) {
    if (es_puma) {
        m->puma_caido = 1;
    } else {
        m->perros_caido = 1;
    }

    pthread_mutex_lock(&log_mutex);
    fprintf(m->salida, "[Mesa %d] (simulacion) el jugador %s falla y sus fichas dejan de moverse\n", m->id,
            es_puma ? "puma" : "perros");
    pthread_mutex_unlock(&log_mutex);
}

// cancela la partida por orden del servidor (Ctrl+C), si la partida ya habia terminado no hace nada
void mesa_cancelar(Mesa *m) {
    pthread_mutex_lock(&m->mutex);

    if (m->partida_activa) {
        m->resultado = CANCELADA;
        m->partida_activa = 0;

        // orden fijo de locks: mesa primero, log despues
        pthread_mutex_lock(&log_mutex);
        fprintf(m->salida, "[Mesa %d] partida cancelada por el servidor (capturas: %d, movimientos: %d)\n", m->id,
                m->tablero.capturas, m->movimientos);
        pthread_mutex_unlock(&log_mutex);
    }

    pthread_mutex_unlock(&m->mutex);
}