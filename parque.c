#include "auxiliar.h"

#define NUM_ENTRADAS 4

#define LOGNAME "parque.log"
#define TERM_VEHICLE_ID -1

/*
 * Struct responsável por armazenar as características do parque.
 */
typedef struct {
  int numLugares;
  int tempoAbertura;
} caracteristicas;

static clock_t ticksInicial;
static int numLugares;
static pthread_mutex_t mutArrumador = PTHREAD_MUTEX_INITIALIZER;
static sem_t *semaforo;
static FILE *parqueLog;
static caracteristicas parqueCar;

/*
 * Retorna o nome do fifo correspondente à orientação pretendida.
 */
char *getFIFOPath (char ori) {

  char* pathname = malloc(strlen("fifo" + 2));

  strncpy(pathname, "fifo", sizeof("fifo"));
  strncat(pathname, &ori, 1);

  return pathname;
}

void *Arrumador(void * arg) {

    pthread_detach(pthread_self()); // Thread do tipo detached.
    int fd;
    infoViatura infoCarro = * (infoViatura *) arg;

    char* pathname = malloc(strlen("fifo" + 3));
    sprintf(pathname, "fifo%d", infoCarro.id);

    if ((fd = open(pathname, O_WRONLY)) == -1) {
      perror(pathname);
      return NULL;
    }

    clock_t ticks = clock() - ticksInicial;

    /*
     * Zona critica: verificação dos lugares vazios.
     */

    pthread_mutex_lock(&mutArrumador);

    if (numLugares > 0) {
        numLugares--;
        pthread_mutex_unlock(&mutArrumador);
        char mensagem[BUF_SIZE];
        sprintf(mensagem, "entrou");
        fprintf(parqueLog, "%-8d ; %-4d ; %-7d ; %s\n", (int) ticks, parqueCar.numLugares - numLugares, infoCarro.id, mensagem);

        write(fd, mensagem, BUF_SIZE);
    }
    else
    {
        pthread_mutex_unlock(&mutArrumador);
        char mensagem[BUF_SIZE];
        sprintf(mensagem, "cheio");
        fprintf(parqueLog, "%-8d ; %-4d ; %-7d ; %s\n", (int) ticks, parqueCar.numLugares - numLugares, infoCarro.id, mensagem);

        write(fd, mensagem, BUF_SIZE);
        close(fd);

        remove(pathname);
        return NULL;
    }

    // Espera um tempo correspondente ao tempo de estacionamento.

    struct timespec tempoEstacionamento;

    tempoEstacionamento.tv_sec = infoCarro.tempoEstacionamento / CLOCKS_PER_SEC;
    tempoEstacionamento.tv_nsec = ((float) infoCarro.tempoEstacionamento / CLOCKS_PER_SEC - tempoEstacionamento.tv_sec) * NSEC_PER_SEC;

    nanosleep(&tempoEstacionamento, NULL);

    /*
     * Zona critica: incrementar o numero de lugares.
     */

    pthread_mutex_lock(&mutArrumador);
    numLugares++;
    pthread_mutex_unlock(&mutArrumador);

    ticks = clock() - ticksInicial;
    char mensagem[BUF_SIZE];
    sprintf(mensagem, "saiu");
    fprintf(parqueLog, "%-8d ; %-4d ; %-7d ; %s\n", (int) ticks, parqueCar.numLugares - numLugares, infoCarro.id, mensagem);

    write(fd, mensagem, BUF_SIZE);
    close(fd);

    remove(pathname);

    pthread_exit(0);
}

void *Controlador (void * arg) {
    char ori = * (int *) arg;
    char* pathname = getFIFOPath(ori);

    if (mkfifo(pathname, PERMISSIONS) == -1) {
      perror(pathname);
      return NULL;
    }

    int fd = open(pathname, O_RDONLY);
    if (fd == -1)
    {
      perror(pathname);
      unlink(pathname);
      return NULL;
    }

    int r;

    while (1)
    {
      infoViatura *info = malloc(sizeof(infoViatura));
      r = read(fd, info, sizeof(*info));

      if (r > 0)
      {
        if (info->id == TERM_VEHICLE_ID)
        {
          free(info);
          break;
        }

        pthread_t id;
        pthread_create(&id, NULL, Arrumador, (void *) info);
      }
      else if (r == -1)
      {
        perror(pathname);
        free(info);
        close(fd);
        unlink(pathname);
        return NULL;
      }
    }

    close(fd);
    remove(pathname);

    pthread_exit(0);
}

int main (int argc, char* argv[]) {

    int tempoAbertura;
    char acessos[4] = "NESO";

    if (argc != 3)
    {
        printf("Usage: %s <N_LUGARES> <T_ABERTURA>\n", argv[0]);
        exit(1);
    }

    // TODO Verificar valor retorno
    numLugares = (int) strtol(argv[1], NULL, 10);
    tempoAbertura = (int) strtol(argv[2], NULL, 10);

    parqueCar.numLugares = numLugares;
    parqueCar.tempoAbertura = tempoAbertura;

    if ((semaforo = sem_open(SEMNAME, O_CREAT, PERMISSIONS, 1)) == SEM_FAILED) {
      perror("Semaforo");
      exit(2);
    }

    pthread_t controladores[NUM_ENTRADAS];

    if ((parqueLog = fopen(LOGNAME, "w")) == NULL) {
      perror(LOGNAME);
      exit(3);
    }

    fprintf(parqueLog, "%-8s ; %-4s ; %-7s ; %s\n", "t(ticks)", "nlug", "id_viat", "observ");

    ticksInicial = clock();

    int i;
    for (i = 0; i < NUM_ENTRADAS; ++i) {
        if (pthread_create(&controladores[i], NULL, Controlador, (void *) &acessos[i]) != 0) {
          perror("Controlador");
          exit(4);
        }
    }

    sleep(parqueCar.tempoAbertura);

    infoViatura *viaturaFecho = malloc(sizeof(infoViatura));
    viaturaFecho->id = TERM_VEHICLE_ID;

    for (i = 0; i < NUM_ENTRADAS; ++i) {
      sem_wait(semaforo);
      int fd;
      char* pathname = malloc(strlen("fifo" + 2));
      pathname = getFIFOPath(acessos[i]);
      if ((fd = open(pathname, O_WRONLY)) == -1) {
        sem_post(semaforo);
        perror(pathname);
        exit(5);
      }
      write(fd, viaturaFecho, sizeof(*viaturaFecho));
      close(fd);
      sem_post(semaforo);
    }

    for (i = 0; i <  NUM_ENTRADAS; i++)
        pthread_join(controladores[i], NULL);

    sem_close(semaforo);
    sem_unlink(SEMNAME);

    pthread_exit(0);
}
