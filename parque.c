#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>

#define N 0
#define E 1
#define S 2
#define O 3
#define NUM_ENTRADAS 4

#define FIFOPER S_IRUSR | S_IWUSR

typedef struct {
  int numLugares;
  int tempoAbertura;
} caracteristicas;

char *getFIFOPath (int num) {
  char ori;
  char* pathname = malloc(strlen("./fifo" + 2));

  switch (num) {
    case N:
      ori = 'N';
      break;
    case E:
      ori = 'E';
      break;
    case S:
      ori = 'S';
      break;
    case O:
      ori = 'O';
      break;
    }

  strncpy(pathname, "./fifo", sizeof("./fifo"));
  strncat(pathname, &ori, 1);

  return pathname;
}

void *Controlador (void * arg) {
    int num = * (int *) arg;
    char* pathname = getFIFOPath(num);
    printf("Thread %d\n", num);

    mkfifo(pathname, FIFOPER);

    pthread_exit(0);
}

void *Parque (void * arg) {
    pthread_t controladores[NUM_ENTRADAS];

    int i;
    for (i = 0; i < NUM_ENTRADAS; ++i) {
        pthread_create(&controladores[i], NULL, Controlador, (void *) &i);
        pthread_join(controladores[i], NULL);
    }

    pthread_exit(0);
}

int main (int argc, char* argv[]) {

    int numLugares;
    int tempoAbertura;

    if (argc != 3)
    {
        printf("Usage: %s <N_LUGARES> <T_ABERTURA>\n", argv[0]);
        exit(1);
    }

    // TODO Verificar valor retorno
    numLugares = (int) strtol(argv[1], NULL, 10);
    tempoAbertura = (int) strtol(argv[2], NULL, 10);

    caracteristicas parqueCar;
    parqueCar.numLugares = numLugares;
    parqueCar.tempoAbertura = tempoAbertura;

    pthread_t parque;
    pthread_create(&parque, NULL, Parque, (void *) &parqueCar);

    pthread_exit(0);
}
