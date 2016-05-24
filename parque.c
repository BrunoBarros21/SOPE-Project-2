#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>

#define N 0
#define E 1
#define S 2
#define O 3
#define NUM_ENTRADAS 4

#define NSEC_PER_SEC  1000000000

#define PERMISSIONS S_IRUSR | S_IWUSR
#define SEMNAME "/Semaforo"
#define LOGNAME "parque.log"
#define BUF_SIZE 16
#define TERM_VEHICLE_ID -1

typedef struct {
    int id;
    char portaAcesso;
    clock_t tempoEstacionamento;
} infoViatura;

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

char *getFIFOPath (char ori) {
  char* pathname = malloc(strlen("fifo" + 2));

  strncpy(pathname, "fifo", sizeof("fifo"));
  strncat(pathname, &ori, 1);

  return pathname;
}

void *Arrumador(void * arg) {
    pthread_detach(pthread_self());
    int fd;
    infoViatura infoCarro = * (infoViatura *) arg;

    char* pathname = malloc(strlen("fifo" + 3));
    sprintf(pathname, "fifo%d", infoCarro.id);

    if ((fd = open(pathname, O_WRONLY)) == -1) {
      perror(pathname);
      return NULL;
    }

    clock_t ticks = clock() - ticksInicial;

    pthread_mutex_lock(&mutArrumador);

    printf("%d\n", numLugares);

    if (numLugares > 0) {
        numLugares--;
        pthread_mutex_unlock(&mutArrumador);
        char mensagem[BUF_SIZE];
        sprintf(mensagem, "entrou");
        printf("Carro %d entrou no estacionamento\n", infoCarro.id);
        fprintf(parqueLog, "%-8d ; %-4d ; %-7d ; %s\n", (int) ticks, parqueCar.numLugares - numLugares, infoCarro.id, mensagem);
        write(fd, mensagem, BUF_SIZE);
    }
    else
    {
        pthread_mutex_unlock(&mutArrumador);
        char mensagem[BUF_SIZE];
        sprintf(mensagem, "cheio");
        printf("Carro %d nao tem lugar disponivel\n", infoCarro.id);
        fprintf(parqueLog, "%-8d ; %-4d ; %-7d ; %s\n", (int) ticks, parqueCar.numLugares - numLugares, infoCarro.id, mensagem);
        write(fd, mensagem, BUF_SIZE);
        close(fd);
        remove(pathname);
        return NULL;
    }

    struct timespec tempoEstacionamento;

    tempoEstacionamento.tv_sec = infoCarro.tempoEstacionamento / CLOCKS_PER_SEC;
    tempoEstacionamento.tv_nsec = ((float) infoCarro.tempoEstacionamento / CLOCKS_PER_SEC - tempoEstacionamento.tv_sec) * NSEC_PER_SEC;

    nanosleep(&tempoEstacionamento, NULL);

    pthread_mutex_lock(&mutArrumador);
    numLugares++;
    pthread_mutex_unlock(&mutArrumador);

    ticks = clock() - ticksInicial;
    char mensagem[BUF_SIZE];
    sprintf(mensagem, "saiu");
    printf("Carro %d saiu do estacionamento\n", infoCarro.id);
    fprintf(parqueLog, "%-8d ; %-4d ; %-7d ; %s\n", (int) ticks, parqueCar.numLugares - numLugares, infoCarro.id, mensagem);
    write(fd, mensagem, BUF_SIZE);
    close(fd);

    remove(pathname);

    pthread_exit(0);
}

void *Controlador (void * arg) {
    char ori = * (int *) arg;
    char* pathname = getFIFOPath(ori);
    printf("Thread %d\n", ori);

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
          printf("Acabou %c.\n", ori);
          free(info);
          break;
        }

        pthread_t id;
        printf("ID: %d\n", info->id);
        //printf("Porta: %c\n", info->portaAcesso);
        printf("Tempo de Estacionamento: %d\n", (int) info->tempoEstacionamento);
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

    printf("Vai fechar o semaforo\n");

    sem_close(semaforo);
    sem_unlink(SEMNAME);

    pthread_exit(0);
}
