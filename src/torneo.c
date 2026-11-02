// torneo.c implementa el servidor del torneo: los jugadores (clientes) llegan, el matchmaker los empareja
// de a dos y les asigna una mesa libre del pool, y un juez por partida inicializa la mesa y la libera al final
// el limite de M mesas simultaneas se controla con un semaforo
// el matchmaker y los jueces se esperan con join, asi main no libera nada que todavia se este usando

#define _DEFAULT_SOURCE
#include "config.h"
#include "mesa.h"
#include "perro.h"
#include "puma.h"
#include "tablero.h"
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define ROL_NINGUNO 0 // el jugador no consiguio pareja
#define ROL_PUMA 1
#define ROL_PERROS 2

typedef struct Partida Partida;

// un jugador (cliente) del torneo
typedef struct {
    int id;           // numero del jugador, de 1 a N
    int rol;          // ROL_PUMA, ROL_PERROS o ROL_NINGUNO
    Partida *partida; // partida en la que juega, NULL si no tiene pareja
    sem_t emparejado; // el jugador espera aqui hasta que lo emparejan
    pthread_t thread;
} Jugador;

// una partida entre dos jugadores en una mesa del pool
struct Partida {
    int numero;                   // numero de la partida, de 1 a N/2
    int slot;                     // posicion de su mesa en el pool
    Jugador *puma;
    Jugador *perros;
    atomic_int jugadores_activos; // jugadores que todavia estan usando la mesa
    pthread_t thread_juez;
};

static Config config;

// pool de mesas, un semaforo cuenta cuantas hay libres
static Mesa *pool_mesas;
static int *mesas_ocupadas;
static sem_t sem_mesas_libres;
static pthread_mutex_t pool_mutex = PTHREAD_MUTEX_INITIALIZER;

// jugadores y cola de espera, los jugadores se encolan al llegar y el matchmaker los saca de a dos
static Jugador *jugadores;
static int *cola;
static int cola_ini = 0;
static int cola_fin = 0;
static pthread_mutex_t cola_mutex = PTHREAD_MUTEX_INITIALIZER;
static sem_t sem_llegadas;

static Partida *partidas;

// cuenta de resultados, indice = resultado de la mesa
static int cuenta_resultados[6];
static pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;

// bandera para apagar el servidor ordenadamente con Ctrl+C
static atomic_int servidor_activo = 1;

// manejador de la senal SIGINT (Ctrl+C)
static void manejador_sigint(int sig) {
    (void)sig;
    // se usa write en vez de printf porque printf no es seguro dentro de una senal
    const char msg[] = "\n[Servidor] Apagando el torneo de forma segura...\n";
    if (write(STDOUT_FILENO, msg, sizeof(msg) - 1) < 0) {
        // ignorar error de write
    }
    servidor_activo = 0;
}

// funcion que espera un semaforo y lo reintenta si una senal la interrumpe
static void esperar_sem(sem_t *s) {
    while (sem_wait(s) == -1 && errno == EINTR) {
        // interrumpido por una senal, se vuelve a esperar
    }
}

// el juez inicializa la mesa, avisa a los jugadores y la libera cuando la partida y las fichas terminaron
static void *rutina_juez(void *arg) {
    Partida *p = arg;
    Mesa *mesa = &pool_mesas[p->slot];

    // por ahora el tope de movimientos y la impresion del tablero usan los valores por defecto (0)
    mesa_iniciar(mesa, p->slot + 1, config.delay_min, config.delay_max, config.seed + p->numero * 1000,
                 0, 0, stdout);

    printf("[Juez] mesa %d: inicia la partida %d (jugador %d puma vs jugador %d perros)\n",
           p->slot + 1, p->numero, p->puma->id, p->perros->id);

    // recien con la mesa lista se avisa a los jugadores para que lancen sus fichas
    sem_post(&p->puma->emparejado);
    sem_post(&p->perros->emparejado);

    // vigilar la partida
    while (mesa->partida_activa && servidor_activo) {
        usleep(100000);
        mesa_revisar_abandono(mesa, 5);
    }

    // si se apago el servidor con la partida en curso, forzar el termino
    if (mesa->partida_activa) {
        mesa->partida_activa = 0;
    }

    // esperar a que los dos jugadores terminen con sus fichas antes de tocar la mesa
    while (atomic_load(&p->jugadores_activos) > 0) {
        usleep(20000);
    }

    int resultado = mesa->resultado;
    printf("[Juez] mesa %d: termina la partida %d (%s)\n", p->slot + 1, p->numero,
           mesa_nombre_resultado(resultado));

    mesa_destruir(mesa);

    pthread_mutex_lock(&stats_mutex);
    cuenta_resultados[resultado]++;
    pthread_mutex_unlock(&stats_mutex);

    // liberar la mesa
    pthread_mutex_lock(&pool_mutex);
    mesas_ocupadas[p->slot] = 0;
    pthread_mutex_unlock(&pool_mutex);

    sem_post(&sem_mesas_libres);
    return NULL;
}

// rutina de cada jugador: llega, se encola, espera pareja y juega con sus fichas
static void *rutina_jugador(void *arg) {
    Jugador *j = arg;
    unsigned int semilla = config.seed + j->id;

    // llegada al torneo en un momento aleatorio
    usleep((rand_r(&semilla) % 200) * 1000);

    pthread_mutex_lock(&cola_mutex);
    cola[cola_fin++] = j->id - 1;
    pthread_mutex_unlock(&cola_mutex);

    printf("[Servidor] llega el jugador %d\n", j->id);
    sem_post(&sem_llegadas);

    // esperar a que el matchmaker lo empareje
    esperar_sem(&j->emparejado);

    if (j->partida == NULL) {
        printf("[Servidor] el jugador %d se retira sin pareja\n", j->id);
        return NULL;
    }

    Partida *p = j->partida;
    Mesa *mesa = &pool_mesas[p->slot];

    if (j->rol == ROL_PUMA) {
        Puma puma;
        puma_iniciar(&puma, mesa);
        puma_esperar(&puma);
    } else {
        Perro perros[NUM_PERROS];
        perros_iniciar(perros, mesa);
        perros_esperar(perros);
    }

    // ultimo uso de la partida por parte del jugador
    atomic_fetch_sub(&p->jugadores_activos, 1);
    return NULL;
}

// funcion que deja a un jugador sin pareja y lo despierta
static void liberar_jugador(Jugador *j) {
    j->rol = ROL_NINGUNO;
    j->partida = NULL;
    sem_post(&j->emparejado);
}

// el matchmaker saca a los jugadores de la cola de a dos, espera una mesa libre y lanza al juez
static void *rutina_matchmaker(void *arg) {
    (void)arg;
    int parejas = config.n_jugadores / 2;
    int creadas = 0;

    for (int k = 0; k < parejas; k++) {
        // esperar a que lleguen dos jugadores
        esperar_sem(&sem_llegadas);
        esperar_sem(&sem_llegadas);

        pthread_mutex_lock(&cola_mutex);
        Jugador *a = &jugadores[cola[cola_ini++]];
        Jugador *b = &jugadores[cola[cola_ini++]];
        pthread_mutex_unlock(&cola_mutex);

        // si las M mesas estan ocupadas la pareja espera aqui sin consumir cpu
        esperar_sem(&sem_mesas_libres);

        int slot = -1;
        pthread_mutex_lock(&pool_mutex);
        for (int i = 0; i < config.m_mesas; i++) {
            if (mesas_ocupadas[i] == 0) {
                mesas_ocupadas[i] = 1;
                slot = i;
                break;
            }
        }
        pthread_mutex_unlock(&pool_mutex);

        // el primero de la cola es el puma y el segundo los perros
        Partida *p = &partidas[creadas];
        p->numero = creadas + 1;
        p->slot = slot;
        p->puma = a;
        p->perros = b;
        atomic_init(&p->jugadores_activos, 2);

        a->rol = ROL_PUMA;
        a->partida = p;
        b->rol = ROL_PERROS;
        b->partida = p;

        printf("[Matchmaker] jugador %d (puma) vs jugador %d (perros) en la mesa %d\n", a->id, b->id, slot + 1);

        if (pthread_create(&p->thread_juez, NULL, rutina_juez, p) != 0) {
            fprintf(stderr, "error al crear el hilo del juez de la mesa %d\n", slot + 1);

            pthread_mutex_lock(&pool_mutex);
            mesas_ocupadas[slot] = 0;
            pthread_mutex_unlock(&pool_mutex);
            sem_post(&sem_mesas_libres);

            liberar_jugador(a);
            liberar_jugador(b);
            continue;
        }

        creadas++;
    }

    // si N es impar el ultimo jugador se queda sin pareja
    if (config.n_jugadores % 2 == 1) {
        esperar_sem(&sem_llegadas);

        pthread_mutex_lock(&cola_mutex);
        Jugador *sobrante = &jugadores[cola[cola_ini++]];
        pthread_mutex_unlock(&cola_mutex);

        liberar_jugador(sobrante);
    }

    // esperar a todos los jueces
    for (int i = 0; i < creadas; i++) {
        pthread_join(partidas[i].thread_juez, NULL);
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s N M [config.txt]\n", argv[0]);
        return 1;
    }

    // capturar Ctrl+C
    struct sigaction sa;
    sa.sa_handler = manejador_sigint;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    grafo_iniciar();

    inicializar_config(&config);

    // por ahora N y M se leen directo de los argumentos, cuando cargar_config este lista se cambia
    int n = atoi(argv[1]);
    int m = atoi(argv[2]);

    if (n <= 0 || m <= 0) {
        fprintf(stderr, "Error: N y M deben ser enteros positivos\n");
        return 1;
    }

    config.n_jugadores = n;
    config.m_mesas = m;

    pool_mesas = calloc((size_t)m, sizeof(Mesa));
    mesas_ocupadas = calloc((size_t)m, sizeof(int));
    jugadores = calloc((size_t)n, sizeof(Jugador));
    cola = calloc((size_t)n, sizeof(int));
    partidas = calloc((size_t)(n / 2 + 1), sizeof(Partida));

    if (!pool_mesas || !mesas_ocupadas || !jugadores || !cola || !partidas) {
        fprintf(stderr, "Error: no se pudo reservar memoria\n");
        free(pool_mesas);
        free(mesas_ocupadas);
        free(jugadores);
        free(cola);
        free(partidas);
        return 1;
    }

    sem_init(&sem_llegadas, 0, 0);
    sem_init(&sem_mesas_libres, 0, (unsigned int)m);

    for (int i = 0; i < n; i++) {
        jugadores[i].id = i + 1;
        jugadores[i].rol = ROL_NINGUNO;
        jugadores[i].partida = NULL;
        sem_init(&jugadores[i].emparejado, 0, 0);
    }

    printf("[Servidor] Torneo iniciado con %d jugadores y %d mesas (semilla %u). Presiona Ctrl+C para detener.\n",
           n, m, config.seed);

    pthread_t matchmaker;
    pthread_create(&matchmaker, NULL, rutina_matchmaker, NULL);

    // lanzar a los jugadores, cada uno llega en un momento aleatorio
    int creados = 0;
    for (int i = 0; i < n; i++) {
        if (!servidor_activo) {
            break;
        }

        if (pthread_create(&jugadores[i].thread, NULL, rutina_jugador, &jugadores[i]) == 0) {
            creados++;
        }
    }

    // esperar solo a los hilos que se crearon
    for (int i = 0; i < creados; i++) {
        pthread_join(jugadores[i].thread, NULL);
    }

    pthread_join(matchmaker, NULL);

    printf("[Servidor] Resumen: %d partidas, %d ganadas por el puma, %d por los perros, %d empates, %d abandonos, %d sin terminar\n",
           cuenta_resultados[GANA_PUMA] + cuenta_resultados[GANA_PERROS] + cuenta_resultados[EMPATE] + 
           cuenta_resultados[GANA_PUMA_ABANDONO] + cuenta_resultados[GANA_PERROS_ABANDONO] + cuenta_resultados[SIN_GANADOR],
           cuenta_resultados[GANA_PUMA], cuenta_resultados[GANA_PERROS], cuenta_resultados[EMPATE],
           cuenta_resultados[GANA_PUMA_ABANDONO] + cuenta_resultados[GANA_PERROS_ABANDONO],
           cuenta_resultados[SIN_GANADOR]);

    for (int i = 0; i < n; i++) {
        sem_destroy(&jugadores[i].emparejado);
    }

    sem_destroy(&sem_llegadas);
    sem_destroy(&sem_mesas_libres);
    pthread_mutex_destroy(&pool_mutex);
    pthread_mutex_destroy(&cola_mutex);
    pthread_mutex_destroy(&stats_mutex);

    free(pool_mesas);
    free(mesas_ocupadas);
    free(jugadores);
    free(cola);
    free(partidas);

    printf("[Servidor] Limpieza completada. Adios.\n");
    return 0;
}