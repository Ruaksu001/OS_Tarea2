#include "config.h"
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void inicializar_config(Config *c) {
    c->n_jugadores = 0;                 // numero de jugadores por defecto = 0
    c->m_mesas = 1;                     // numero de mesas por defecto = 1
    c->delay_min = 50;                  // retardo minimo por defecto = 50ms
    c->delay_max = 300;                 // retardo maximo por defecto = 300ms
    c->seed = (unsigned int)time(NULL); // la seed se genera en base al tiempo
    c->max_movimientos = 1000;          // tope de movimientos por defecto = 1000
    c->imprimir_cada = 50;              // imprimir el tablero cada 50 movimientos por defecto
    c->abandono_jugador = 0;            // por defecto nadie abandona
    c->abandono_ms = 2000;              // por defecto el abandono ocurre a los 2 segundos de empezar la partida
    c->tiene_disposicion = 0;           // por defecto se usa la disposicion inicial normal
}

// NOTA: parsear_entero, parsear_seed, parsear_lista y parse_config se encargan de que
// el programa no acepte valores erroneos en config.txt
// basicamente el manejador de problemas de capa 8.

// manejar valores numericos incorrectos en config.txt
static int parsear_entero(const char *str, int *salida, int min_val, int max_val) {

    char *endptr;
    errno = 0; // variable global que entrega error que ocurre en una funcion
    long val = strtol(str, &endptr, 10);

    // ocurre cuando se ingresan valores erroneos (no numericos o vacios) como parametro
    if (endptr == str) {
        return 0;
    }

    // saltar espacios despues del numero
    while (*endptr == ' ' || *endptr == '\t' || *endptr == '\r' || *endptr == '\n') {
        endptr++;
    }

    // ignora todo despues del valor del parametro
    if (*endptr != '\0') {
        return 0;
    }

    // detectar overflows o valores mas grandes que los designados
    if (errno == ERANGE || val < min_val || val > max_val) {
        return 0;
    }

    *salida = (int)val;
    return 1;
}

// manejar valores del parametro seed en config.txt
static int parsear_seed(const char *str, unsigned int *salida) {

    char *endptr;
    errno = 0;
    unsigned long val = strtoul(str, &endptr, 10);

    // no se lee ningun numero
    if (endptr == str)
        return 0;

    // saltar espacios, tabulaciones o saltos de linea
    while (*endptr == ' ' || *endptr == '\t' || *endptr == '\r' || *endptr == '\n') {
        endptr++;
    }

    // limpiar parametro de caracteres invalidos, detectar overflows y mantener
    // val dentro de valores que no provoquen incendios indeseados en el computador del operador
    if (*endptr != '\0' || errno == ERANGE || val > UINT_MAX) {
        return 0;
    }

    *salida = (unsigned int)val;
    return 1;
}

// manejar listas de enteros separados por coma (por ejemplo perros=15,16,17), tiene que haber exactamente cantidad numeros
static int parsear_lista(const char *str, int *salida, int cantidad, int min_val, int max_val) {

    const char *ptr = str;

    for (int i = 0; i < cantidad; i++) {
        char *endptr;
        errno = 0;
        long val = strtol(ptr, &endptr, 10);

        // no se leyo un numero o esta fuera de rango
        if (endptr == ptr || errno == ERANGE || val < min_val || val > max_val) {
            return 0;
        }

        salida[i] = (int)val;
        ptr = endptr;

        while (*ptr == ' ' || *ptr == '\t') {
            ptr++;
        }

        // entre un numero y otro tiene que haber una coma
        if (i < cantidad - 1) {
            if (*ptr != ',') {
                return 0;
            }
            ptr++;
        }
    }

    // no puede sobrar nada despues del ultimo numero
    while (*ptr == ' ' || *ptr == '\t' || *ptr == '\r' || *ptr == '\n') {
        ptr++;
    }

    return *ptr == '\0';
}

// avisar que un valor de config.txt no se pudo usar
static void aviso_valor(const char *setting) {
    fprintf(stderr, "Aviso: valor invalido para '%s' en config.txt, se usa el valor por defecto\n", setting);
}

// verificar formato correcto de config.txt
static int parse_config(Config *c, const char *ruta) {

    FILE *f = fopen(ruta, "r"); // leer el archivo desde la ruta ingresada como parametro
    if (!f) {
        perror("Error al ejecutar config.txt");
        return 0;
    }

    // la disposicion inicial se arma con dos claves (puma y perros), se valida al final cuando estan las dos
    int pos_puma = -1;
    int pos_perros[NUM_PERROS];
    int tiene_perros = 0;

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
            if (!parsear_entero(val, &c->delay_min, 1, 10000)) { // los delays varian entre 1ms y 10000ms
                aviso_valor(setting);
            }
        } else if (strcmp(setting, "delay_max") == 0) {
            if (!parsear_entero(val, &c->delay_max, 1, 10000)) {
                aviso_valor(setting);
            }
        } else if (strcmp(setting, "seed") == 0) {
            if (!parsear_seed(val, &c->seed)) {
                aviso_valor(setting);
            }
        } else if (strcmp(setting, "max_movimientos") == 0) {
            if (!parsear_entero(val, &c->max_movimientos, 1, 1000000)) {
                aviso_valor(setting);
            }
        } else if (strcmp(setting, "imprimir_cada") == 0) {
            if (!parsear_entero(val, &c->imprimir_cada, 1, 1000000)) {
                aviso_valor(setting);
            }
        } else if (strcmp(setting, "abandono_jugador") == 0) {
            if (!parsear_entero(val, &c->abandono_jugador, 0, 1000000)) { // 0 significa que nadie abandona
                aviso_valor(setting);
            }
        } else if (strcmp(setting, "abandono_ms") == 0) {
            if (!parsear_entero(val, &c->abandono_ms, 0, 600000)) {
                aviso_valor(setting);
            }
        } else if (strcmp(setting, "puma") == 0) {
            if (!parsear_entero(val, &pos_puma, 0, NUM_NODOS - 1)) {
                pos_puma = -1;
                aviso_valor(setting);
            }
        } else if (strcmp(setting, "perros") == 0) {
            tiene_perros = parsear_lista(val, pos_perros, NUM_PERROS, 0, NUM_NODOS - 1);
            if (!tiene_perros) {
                aviso_valor(setting);
            }
        } else {
            fprintf(stderr, "Aviso: clave desconocida '%s' en config.txt, se ignora\n", setting);
        }
    }

    fclose(f);

    // armar la disposicion inicial si se entregaron el puma y los 12 perros
    if (pos_puma >= 0 && tiene_perros) {
        int pos[NUM_FICHAS];
        pos[ID_PUMA] = pos_puma;
        for (int i = 0; i < NUM_PERROS; i++) {
            pos[i + 1] = pos_perros[i];
        }

        if (tablero_validar_posiciones(pos)) {
            for (int i = 0; i < NUM_FICHAS; i++) {
                c->pos_inicial[i] = pos[i];
            }
            c->tiene_disposicion = 1;
        } else {
            fprintf(stderr, "Aviso: disposicion inicial invalida (nodos repetidos), se usa la disposicion por defecto\n");
        }
    } else if (pos_puma >= 0 || tiene_perros) {
        fprintf(stderr, "Aviso: para la disposicion inicial hacen falta las claves 'puma' y 'perros', se usa la disposicion por defecto\n");
    }

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
    int archivo_cargado = 0;
    if (argc >= 4) {
        if (!parse_config(c, argv[3])) {
            fprintf(stderr,
                    "No se pudo leer archivo de configuracion '%s', usando valores "
                    "por defecto.\n",
                    argv[3]);
        } else {
            archivo_cargado = 1;
        }
    }

    if (c->delay_min < 0) {
        c->delay_min = 50;
    }

    if (c->delay_max < c->delay_min) {
        c->delay_max = c->delay_min;
    }

    // el jugador que abandona tiene que existir
    if (c->abandono_jugador > n) {
        fprintf(stderr, "Aviso: abandono_jugador=%d no existe (N=%d), no abandona nadie\n", c->abandono_jugador, n);
        c->abandono_jugador = 0;
    }

    if (archivo_cargado) {
        printf(
                "Archivo '%s' cargado con exito (parámetros no ingresados o comentados se inicializan por defecto):\n",
                argv[3]);
        printf("    - delay_min: %d ms\n", c->delay_min);
        printf("    - delay_max: %d ms\n", c->delay_max);
        printf("    - seed: %u\n", c->seed);
        printf("    - max_movimientos: %d\n", c->max_movimientos);
        printf("    - imprimir_cada: %d\n", c->imprimir_cada);

        if (c->abandono_jugador > 0) {
            printf("    - abandono: el jugador %d falla a los %d ms de empezar su partida\n", c->abandono_jugador,
                   c->abandono_ms);
        }

        if (c->tiene_disposicion) {
            printf("    - disposicion inicial: cargada desde el archivo\n");
        }
    }

    return 1;
}