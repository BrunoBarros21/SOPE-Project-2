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

#define PERMISSIONS S_IRUSR | S_IWUSR
#define SEMNAME "/Semaforo"
#define LOGNAME "gerador.log"
#define BUF_SIZE 16

unsigned int idViatura;

typedef struct {
    int id;
    char portaAcesso;
    clock_t tempoEstacionamento;
} infoViatura;

static sem_t *semaforo;
static FILE *parqueLog;
static clock_t ticksInicial;

void *gestaoEntrada(void * arg) {
    pthread_detach(pthread_self());
    clock_t tempoInicial = clock();
    clock_t ticks;
    infoViatura * info = (infoViatura *) arg;
    //printf("Entrada: %c\nTempo Estacionamento: %f\nID: %d\n\n", info->portaAcesso, (float)info->tempoEstacionamento / CLOCKS_PER_SEC, info->id);
    printf("Começou a viatura %d\n", info->id);
    char pathname[7];
    sprintf(pathname, "fifo%c", info->portaAcesso);

    char fifoPrivado[20];
    sprintf(fifoPrivado, "fifo%d", info->id);

    if (mkfifo(fifoPrivado, PERMISSIONS) == -1) {
      perror(fifoPrivado);
      return NULL;
    }

    printf("Chegou aos semaforos da viatura %d\n", info->id);

    sem_wait(semaforo);
    int fd;
    if ((fd = open(pathname, O_WRONLY)) == -1) {
      sem_post(semaforo);
      unlink(fifoPrivado);
      printf("Erro no open da viatura %d\n", info->id);
      free(info);
      return NULL;
    }
    if (write(fd, info, sizeof(infoViatura)) == -1) {
      sem_post(semaforo);
      close(fd);
      remove(fifoPrivado);
      printf("Erro no write da viatura %d\n", info->id);
      free(info);
      return NULL;
    }
    close(fd);
    sem_post(semaforo);

    printf("Passou os semaforos da viatura %d\n", info->id);

    fd = open(fifoPrivado, O_RDONLY);
    char mensagem[BUF_SIZE];
    read(fd, mensagem, BUF_SIZE);

    if (strcmp(mensagem, "entrou") == 0) {
      ticks = clock() - ticksInicial;
      fprintf(parqueLog, "%-8d ; %-7d ; %-6c ; %-10d ; %-6c ; %s\n", (int) ticks, info->id, info->portaAcesso, (int) info->tempoEstacionamento, '?', mensagem);

      read(fd, mensagem, BUF_SIZE);
      ticks = clock() - ticksInicial;
      clock_t tempoVida = clock() - tempoInicial;
      fprintf(parqueLog, "%-8d ; %-7d ; %-6c ; %-10d ; %-6d ; %s\n", (int) ticks, info->id, info->portaAcesso, (int) info->tempoEstacionamento, (int) tempoVida, mensagem);
    }
    else if (strcmp(mensagem, "cheio") == 0) {
      ticks = clock() - ticksInicial;
      fprintf(parqueLog, "%-8d ; %-7d ; %-6c ; %-10d ; %-6c ; %s\n", (int) ticks, info->id, info->portaAcesso, (int) info->tempoEstacionamento, '?', mensagem);
    }

    close(fd);

    printf("Acabou a viatura %d\n", info->id);

    free(info);

    return NULL;
}

int main (int argc, char* argv[]) {
    int tempoGeracao;
    int uniRelogio;
    char acessos[4] = {'N', 'E', 'S', 'O'};
    int probabilidades[10] = {0, 0, 0, 0, 0, 1, 1, 1, 2, 2};
    idViatura = 1;

    if (argc != 3)
    {
        printf("Usage: %s <T_GERACAO> <U_RELOGIO>\n", argv[0]);
        exit(1);
    }

    tempoGeracao = (int) strtol(argv[1], NULL, 10);
    uniRelogio = (int) strtol(argv[2], NULL, 10);

    if ((semaforo = sem_open(SEMNAME, O_CREAT, PERMISSIONS, 1)) == SEM_FAILED) {
      perror("Semaforo");
      exit(2);
    }

    if ((parqueLog = fopen(LOGNAME, "w")) == NULL) {
      perror(LOGNAME);
      exit(3);
    }

    fprintf(parqueLog, "%-8s ; %-7s ; %-6s ; %-10s ; %-6s ; %s\n", "t(ticks)", "id_viat", "destin", "t_estacion", "t_vida", "observ");

    srand(time(NULL));

    clock_t duracao = tempoGeracao * CLOCKS_PER_SEC;
    clock_t fim;
    clock_t inicio = clock();
    clock_t anterior = inicio;

    int indiceProximo;
    int proximo = 0;

    do {
        fim = clock();

        if ((fim - anterior) >= proximo) {
            int indiceAcesso = rand() % 4;
            int tempoEstacionamento = rand() % 10 + 1;

            infoViatura *viatura = malloc(sizeof(viatura));
            viatura->portaAcesso = acessos[indiceAcesso];
            viatura->tempoEstacionamento = tempoEstacionamento * uniRelogio;
            viatura->id = idViatura++;

            //printf("%d\n", viatura->id);

            pthread_t entrada;
            pthread_create(&entrada, NULL, gestaoEntrada, (void *) viatura);

            anterior = fim;

            indiceProximo = rand() % 10;
            proximo = probabilidades[indiceProximo] * uniRelogio;

            //printf("%d\n", proximo);
        }

    } while(fim - inicio < duracao);

    printf("vai fechar semaforo\n");

    sem_close(semaforo);
    sem_unlink(SEMNAME);

    printf("vai sair\n");

    pthread_exit(0);
  }
