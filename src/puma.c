#define _DEFAULT_SOURCE
#include "puma.h"
#include "tablero.h"
#include <stdlib.h>
#include <unistd.h>

void *rutina_puma(void *arg) {
  Puma *p = arg;
  unsigned int seed = p->semilla; // semilla local del hilo para la aleatoriedad

  while (*p->partida_activa) {

    // esperar el rango definido con variables de delay
    int rango = p->delay_max - p->delay_min + 1;
    int tiempo = p->delay_min + (rand_r(&seed) % rango);
    usleep(tiempo * 1000);

    if (!*p->partida_activa) {
      break;
    }

    // NOTA: hasta aqui llego hoy, son las 3am asi que me voy a dormir zzzzzzz
    // NOTA: traiganle una falda a la niña
  }
  
  return NULL;
}

// inicializa y ejecuta el thread del puma
void puma_iniciar(Puma *puma, Tablero *t, pthread_mutex_t *mutex,
                  volatile int *partida_activa, int delay_min, int delay_max,
                  unsigned int semilla_base) {

  puma->id = ID_PUMA;
  puma->tablero = t;
  puma->mutex = mutex;
  puma->partida_activa = partida_activa;
  puma->delay_min = delay_min;
  puma->delay_max = delay_max;
  puma->semilla = semilla_base + puma->id; // guarda la semilla base mas la id del puma

  pthread_create(&puma->thread, NULL, rutina_puma, puma);
}

// esperar a que termine el hilo del puma
void puma_esperar(Puma *puma) { pthread_join(puma->thread, NULL); }