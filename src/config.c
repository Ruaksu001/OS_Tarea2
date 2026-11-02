#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

void inicializar_config(Config *c) {
    c->n_jugadores = 0;                 // numero de jugadores por defecto = 0
    c->m_mesas = 1;                     // numero de mesas por defecto = 1
    c->delay_min = 50;                  // retardo minimo por defecto = 50ms
    c->delay_max = 300;                 // retardo maximo por defecto = 300ms
    c->seed = (unsigned int)time(NULL); // la seed se genera en base al tiempo
}

static int parse_config(Config *c, const char *ruta) {

    FILE *f = fopen(ruta, "r"); // leer el archivo desde la ruta ingresada como parametro
    if (!f) {
        perror("Error al ejecutar config.txt");
        return 0;
    }

    // buffer que almacena cada linea
    char linea[256];

    // ciclo que lee linea por linea hasta EOF
    while (fgets(linea, sizeof(linea), f)) {

        // para recorrer los caracteres se usa un puntero
        char *ptr = linea;

        // el puntero se salta los espacios y los tabs
        while (*ptr == ' ' || *ptr == '\t') {
            ptr++;
        }

        // ignorar comentarios, saltos de linea y fin de cadena \0
        if (*ptr == '#' || *ptr == '\n' || *ptr == '\0') {
            continue;
        }

        char setting[64]; // nombre del setting (configuracion)

        // extraer nombre del setting hasta el signo = o un espacio y descartar
        // lineas con mal formato
        if (sscanf(ptr, "%63[^= \t]", setting) != 1) {
            continue;
        }

        char *igual = strchr(ptr, '='); // primera ocurrencia de = en la linea
        if (!igual)
            continue;
        char *val = igual + 1; // el valor empieza despues del =

        if (strcmp(setting, "delay_min") == 0) {
            c->delay_min = atoi(val);
        } else if (strcmp(setting, "delay_max") == 0) {
            c->delay_max = atoi(val);
        } else if (strcmp(setting, "seed") == 0) {
            c->seed = (unsigned int)strtoul(val, NULL, 10);
        }
    }

    fclose(f);
    return 1;
}

int cargar_config(Config *c, int argc, char *argv[]) {

    if (!c)
        return 0;

    // cargar valores por defecto
    inicializar_config(c);

    // validar que el input tenga al menos n y m
    if (argc < 3) {
        fprintf(stderr, "Uso %s N M [config.txt]\n", argv[0]);
        return 0;
    }

    // recibir variables n y m de argumento
    int n = atoi(argv[1]);
    int m = atoi(argv[2]);

    if (n <= 0 || m <= 0) {
        fprintf(stderr, "Error: N y M deben ser enteros positivos mayores a 0\n");
        return 0;
    }

    c->n_jugadores = n;
    c->m_mesas = m;

    // en caso de que se entregue un config.txt
    // es opcional por lo que el programa se ejecuta igual
    if (argc >= 4) {
        if (!parse_config(c, argv[3])) {
            fprintf(stderr,
                    "No se pudo leer archivo de configuracion '%s', usando valores "
                    "por defecto.\n",
                    argv[3]);
        }
    }

    if (c->delay_min < 0) {
        c->delay_min = 50;
    }

    if (c->delay_max < c->delay_min) {
        c->delay_max = c->delay_min;
    }

    return 1;
}