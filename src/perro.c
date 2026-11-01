#include "perro.h"
#include "tablero.h"
#include <stdlib.h>
#include <unistd.h>

// funcion que retorna la cantidad de movimientos posibles de un perro
// llena el arreglo destinos_validos con los nodos a los que el perro se puede
// mover
int obtener_movimientos_perro(const Tablero *t, int id_perro,
                              int destinos_validos[MAX_VECINOS]) {

  // verificar que el perro es valido
  if (id_perro < 1 || id_perro > NUM_PERROS) {
    return 0;
  }

  int origen = t->pos_ficha[id_perro];
  if (origen == CAPTURADO)
    return 0;

  int total = 0;
  int n_vecinos =
      cantidad_vecinos(origen); // saca numero de vecinos segun valor de
                                // adyacencia de la posicion de origen

  for (int i = 0; i < n_vecinos; i++) {
    // se busca la id del vecino numero i desde el origen para buscarlo en el
    // tablero
    int vecino = vecino_de(origen, i);
    if (t->casilla[vecino] == VACIO) {
      destinos_validos[total] = vecino; // guarda casilla libre en la lista
      total++;                          // aumenta la cantidad de vecinos libres
    }
  }

  // retorna total de vecinos libres
  return total;
}

// funcion que mueve al perro hacia una casilla entregada como parametro
// (destino)
int mover_perro(Tablero *t, int id_perro, int destino) {
  if (id_perro < 1 || id_perro > NUM_PERROS)
    return 0;

  int origen = t->pos_ficha[id_perro];
  if (origen == CAPTURADO)
    return 0;

  // verificar que el destino sea adyacente y este libre
  if (!son_vecinos(origen, destino) || t->casilla[destino] != VACIO) {
    return 0;
  }

  // actualizar el estado del tablero y posicion de la ficha
  t->casilla[origen] = VACIO;
  t->casilla[destino] = PERRO;
  t->pos_ficha[id_perro] = destino;

  // retorna 1 si tiene exito
  return 1;
}

// rutina que cada perro ejecuta continuamente de forma paralela (concurrente)
void *rutina_perro(void *arg) {
  Perro *p = arg;

  while (*p->partida_activa) {

    // esperar entre 50ms y 300ms
    int rango = p->delay_max - p->delay_min + 1;
    int tiempo_ms = p->delay_min + (rand() % rango);
    usleep(tiempo_ms * 1000);

    // seccion critica, se protege el tablero con mutex
    pthread_mutex_lock(p->mutex);

    // si el perro es capturado mientras espera, se termina el thread
    if (p->tablero->pos_ficha[p->id] == CAPTURADO) {
      pthread_mutex_unlock(p->mutex);
      break;
    }

    // si no es capturado, se mueve a una posicion valida al azar
    int opciones[MAX_VECINOS];
    int cantidad = obtener_movimientos_perro(p->tablero, p->id, opciones);

    // si cantidad es mayor a 0 quiere decir que existe al menos una
    // posicion valida para moverse
    if (cantidad > 0) {
      int destino = opciones[rand() % cantidad];
      mover_perro(p->tablero, p->id, destino);
    }

    // se libera el mutex al finalizar el movimiento
    pthread_mutex_unlock(p->mutex);
  }

  return NULL;
}

// inicializa los perros y crea threads correspondientes
void perros_iniciar(Perro perros[NUM_PERROS], Tablero *t,
                    pthread_mutex_t *mutex, volatile int *partida_activa,
                    int delay_min, int delay_max) {

  for (int i = 0; i < NUM_PERROS; i++) {
    perros[i].id = i + 1; // la id va del 1 al 12
    perros[i].tablero = t;
    perros[i].mutex = mutex;
    perros[i].partida_activa = partida_activa;
    perros[i].delay_min = delay_min;
    perros[i].delay_max = delay_max;

    pthread_create(&perros[i].thread, NULL, rutina_perro, &perros[i]);
  }
}

// espera a que los 12 threads terminen
void perros_esperar(Perro perros[NUM_PERROS]) {
  for (int i = 0; i < NUM_PERROS; i++) {
    pthread_join(perros[i].thread, NULL);
  }
}