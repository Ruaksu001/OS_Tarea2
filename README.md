# Torneo de Komikan Concurrente

## Descripción

Este proyecto consiste en un simulador y servidor concurrente para torneos multijugador del tradicional juego mapuche **Komikan**, desarrollado en lenguaje C (estándar C17) utilizando primitivas de sincronización POSIX (`pthread`, mutexes y semáforos).

El juego enfrenta a un Puma contra 12 Perros en un tablero asimétrico de 28 nodos (una cuadrícula central de $5 \times 5$ intersecciones acoplada en su extremo superior al triángulo o trono). A diferencia de los juegos de mesa por turnos estrictos, en este torneo todas las piezas compiten y se mueven en tiempo real de forma simultánea e independiente en intervalos de milisegundos (entre 50 y 300 ms por defecto).

El sistema modela la infraestructura completa del torneo:
1. **Servidor y Matchmaker:** Acepta la llegada concurrente de $N$ clientes, los encola y los empareja de a dos (un jugador Puma y un jugador Perros).
2. **Control de Capacidad:** Administra un pool de mesas activas garantizando que nunca se supere el límite de $M$ mesas simultáneas, haciendo que las parejas restantes esperen de forma pasiva mediante semáforos POSIX.
3. **Árbitro / Juez de Mesa:** Cada partida activa cuenta con un hilo Juez independiente que inicializa el tablero, supervisa la validez de los movimientos, registra el log en tiempo real, detecta victorias o abandonos por inactividad y libera los recursos al concluir.
4. **Resiliencia:** Manejo limpio de señales POSIX (`SIGINT` / Ctrl+C) para detener la admisión de jugadores, concluir o cancelar las partidas activas y liberar todos los recursos sin fugas de memoria (*zero memory leaks*) ni hilos huérfanos.

---

## Modo de uso

El programa recibe por línea de comandos el número total de jugadores a procesar ($N$), el límite máximo de mesas concurrentes ($M$) y, de forma opcional, la ruta a un archivo de configuración (`config.txt`):

```bash
./torneo N M [config.txt]
```

### Ejemplos de ejecución

* **Partida básica (2 jugadores, 1 mesa, configuración por defecto):**
  ```bash
  ./torneo 2 1
  ```

* **Torneo mediano con concurrencia controlada (10 jugadores, máximo 2 mesas activas):**
  ```bash
  ./torneo 10 2
  ```

* **Torneo con archivo de configuración personalizado:**
  ```bash
  ./torneo 40 4 config.txt
  ```

* **Jugadores impares (el jugador sobrante se retira limpiamente sin pareja):**
  ```bash
  ./torneo 7 2
  ```

---

## Formato de config.txt

```ini
# Configuración del Torneo de Komikan
delay_min = 30
delay_max = 120
seed = 42
max_movimientos = 1500
imprimir_cada = 50
abandono_jugador = 0
abandono_ms = 2000

# Disposición inicial personalizada (opcional):
# puma = 27
# perros = 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 10, 14
```

### Campos admitidos:
* **`delay_min`:** Retardo mínimo de decisión de cada ficha en milisegundos (por defecto: 50 ms).
* **`delay_max`:** Retardo máximo de decisión de cada ficha en milisegundos (por defecto: 300 ms).
* **`seed`:** Semilla numérica entera para reproducibilidad de pruebas y números pseudoaleatorios. Si no se especifica, se genera en base a `time(NULL)`.
* **`max_movimientos`:** Límite máximo de movimientos permitidos antes de declarar la partida en empate (por defecto: 1000).
* **`imprimir_cada`:** Frecuencia con la que el Juez imprime el estado visual de la matriz en consola (por defecto: cada 50 movimientos).
* **`abandono_jugador`:** ID del jugador (1 a $N$) al cual se le simulará una falla o desconexión (`0` = nadie abandona).
* **`abandono_ms`:** Tiempo en milisegundos tras el cual el jugador configurado deja de responder.
* **`puma`:** Índice del nodo inicial del Puma (0 a 27).
* **`perros`:** Lista de 12 enteros separados por comas que indican los nodos donde inician los perros.

---

## Archivos del Proyecto

El código fuente está estructurado de manera modular dentro del directorio `src/`:

### `tablero.c` / `tablero.h`
Modela el estado del juego y la geometría del tablero como un grafo de 28 nodos (casillas 0 a 24 para la cuadrícula $5 \times 5$; 25 y 26 para los nodos intermedios y 27 para el Trono):
* `grafo_iniciar()`: Construye el grafo de adyacencias ortogonales y las conexiones diagonales/verticales del trono.
* `saltos_iniciar()`: Precomputa en una tabla estática todas las líneas rectas de 3 nodos posibles para los saltos de captura del Puma.
* `mover_puma()` / `mover_perro()`: Aplica las transiciones de casillas (`VACIO`, `PUMA`, `PERRO`, `CAPTURADO`).
* `tablero_ganador()`: Evalúa las condiciones de término (6 o más capturas para el Puma; acorralamiento sin movimientos válidos para los Perros).
* `imprimir_tablero()`: Dibuja la representación visual del tablero en consola o archivo log.

### `perro.c` / `perro.h`
Gestiona el comportamiento concurrente de los 12 perros:
* `rutina_perro()`: Función ejecutada por cada uno de los 12 hilos. Duerme un retardo aleatorio entre `delay_min` y `delay_max`, pide movimiento a la mesa y se auto-termina si el perro es capturado o si la partida concluye.
* `perros_iniciar()` / `perros_esperar()`: Crea y sincroniza el término (`pthread_join`) de los 12 hilos de la jauría.

### `puma.c` / `puma.h`
Controla el hilo del Puma:
* `rutina_puma()`: Ejecuta el ciclo de vida del Puma. Prioriza saltar y capturar perros sobre casillas adyacentes; si no hay capturas disponibles, ejecuta un paso simple a una casilla vacía.
* `puma_iniciar()` / `puma_esperar()`: Inicializa y espera la finalización del hilo.

### `mesa.c` / `mesa.h`
Encapsula el estado de una partida activa y su sincronización:
* `mesa_mover_puma()` / `mesa_mover_perro()`: Secciones críticas atómicas protegidas por el mutex de la mesa (`m->mutex`).
* `mesa_revisar_abandono()`: Compara el reloj monotónico (`clock_gettime`) contra los últimos latidos registrados (`latido_puma_ms`, `latido_perros_ms`). Si la inactividad supera el umbral dinámico, finaliza la partida declarando ganador al oponente por abandono.
* `mesa_dormir()`: Permite dormir en tramos pequeños de 20 ms para reaccionar inmediatamente ante cancelaciones sin retardar el apagado del servidor.
* `mesa_cancelar()`: Marca la partida como cancelada ante una señal `SIGINT`.

### `config.c` / `config.h`
Módulo encargado de parsear y sanitizar la configuración:
* `parsear_entero()` / `parsear_seed()` / `parsear_lista()`: Conversiones numéricas seguras mediante `strtol` y `strtoul`, validando desbordamientos de rango (`errno == ERANGE`), caracteres espurios y valores negativos.
* `cargar_config()`: Valida los argumentos CLI, aplica valores por defecto y procesa el archivo opcional.

### `torneo.c`
Punto de entrada principal del servidor (`main`):
* `rutina_jugador()`: Modela la llegada independiente de cada cliente, su encolamiento y la creación de sus piezas según el rol asignado.
* `rutina_matchmaker()`: Extrae parejas de la cola, espera de forma pasiva por una mesa libre (`sem_mesas_libres`) y lanza el hilo Juez.
* `rutina_juez()`: Inicializa la mesa, vigila periódicamente el estado, registra el resultado y libera la mesa para nuevos jugadores.
* `manejador_sigint()`: Manejador de señal `SIGINT` (Ctrl+C) que detiene nuevas admisiones, cancela partidas en curso y garantiza la liberación total de recursos.

---

## Compilación y Ejecución

Desde la raíz del repositorio, compilar con GCC utilizando las banderas exigidas por el enunciado:

```bash
gcc -Wall -Wextra -std=c17 -pthread src/tablero.c src/perro.c src/puma.c src/mesa.c src/config.c src/torneo.c -o torneo
```

Para ejecutar el torneo:

```bash
./torneo 10 2 config.txt
```

---

## Decisiones de Diseño

### 1. Precomputación estática de la tabla de saltos del Puma
En lugar de calcular fórmulas de geometría analítica o comprobar matrices en cada ciclo de decisión, todos los saltos en línea recta válidos (horizontales, verticales y trayectorias del trono) se precomputan una sola vez al iniciar el programa en `saltos_iniciar()`. Consultar si un salto es legal es una operación $O(1)$ sin sobrecarga matemática en tiempo de ejecución.

### 2. Jerarquía estricta de bloqueos (Prevención total de Deadlocks)
Para evitar interbloqueos (*deadlocks*), el sistema utiliza un orden canónico estricto de adquisición de primitivas:
1. `m->mutex` (protege el estado del tablero y los contadores de la mesa).
2. `log_mutex` (protege la escritura sincronizada en el flujo de salida).
`log_mutex` nunca se solicita antes que el mutex de mesa ni se adquieren dos mutexes de mesas distintas a la vez, eliminando de raíz cualquier posibilidad de espera circular (*Coffman Deadlock Condition*).

### 3. Exclusión mutua a nivel de Mesa vs. por Casilla
Se optó por proteger la mesa completa con un `pthread_mutex_t` por cada partida en lugar de un cerrojo independiente por casilla. Esto simplifica drásticamente la atomicidad del salto de captura (que involucra simultáneamente tres nodos: origen, casilla saltada y destino), garantizando un rendimiento óptimo con cero posibilidades de interbloqueo y sin sobrecargar al planificador del sistema operativo con decenas de mutexes compitiendo en milisegundos.

### 4. Sincronización pasiva sin espera activa (*Busy-Waiting*)
Para no saturar la CPU con bucles continuos de consulta, el emparejamiento y el control del límite de $M$ mesas concurrentes se implementan mediante **semáforos POSIX** (`sem_t`):
* `sem_llegadas`: El Matchmaker se bloquea pasivamente hasta que haya suficientes jugadores en cola.
* `sem_mesas_libres`: Se inicializa con el valor $M$. Si las $M$ mesas están activas, las parejas en espera quedan suspendidas por el kernel hasta que un Juez finalice una partida y ejecute `sem_post`.

### 5. Umbral de abandono dinámico y reloj monotónico
Para evitar falsos positivos si un usuario configura retardos elevados (por ejemplo, `delay_max = 6000 ms`), el tiempo límite de abandono se calcula dinámicamente como al menos `(delay_max / 1000) * 5` segundos. Además, el seguimiento de inactividad se realiza mediante `clock_gettime(CLOCK_MONOTONIC)`, inmune a ajustes horarios o cambios de fecha en el sistema operativo.

### 6. Cierre limpio e inmediato con `SIGINT` (Ctrl+C)
Ante la interrupción por teclado, el manejador instala `servidor_activo = 0` y emite `sem_post` sobre los semáforos bloqueantes. La función `esperar_sem_cancelable()` despierta inmediatamente al Matchmaker, la cola de espera se sella liberando a los jugadores no emparejados, las mesas activas se cancelan y `pthread_join` asegura que ningún hilo quede huérfano antes de destruir las primitivas y liberar la memoria dinámica.

---

## Limitaciones Actuales

* **Jugadores impares:** Si el torneo se inicia con un número $N$ impar de jugadores, el último jugador en la cola de espera se retira limpiamente del servidor sin pareja, ya que el juego requiere obligatoriamente 2 jugadores (1 Puma y 1 Perros).
* **Escalabilidad por hilos:** Cada mesa activa genera 14 hilos concurrentes (1 Juez, 1 Puma y 12 Perros). Para valores extremadamente altos de $M$ (por ejemplo, $M > 500$ mesas activas en paralelo), el sistema podría aproximarse al límite de hilos por usuario (`ulimit -u`) del sistema operativo anfitrión.