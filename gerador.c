#include "auxiliar.h"

#define LOGNAME "gerador.log"

unsigned int idViatura;

static sem_t *semaforo;
static FILE *parqueLog;
static clock_t ticksInicial;

/*
 * Escreve para o ficheiro .log as informações pretendidas.
 * Caso o tempo de vida seja -1, imprime '?'.
 */
void logGerador(int ticks, int id, char porta, int tempoEstacionamento, int tempoVida, char* mensagem) {
  if (tempoVida == -1)
    fprintf(parqueLog, "%-8d ; %-7d ; %-6c ; %-10d ; %-8c ; %s\n", ticks, id, porta, tempoEstacionamento, '?', mensagem);
  else
    fprintf(parqueLog, "%-8d ; %-7d ; %-6c ; %-10d ; %-8d ; %s\n", ticks, id, porta, tempoEstacionamento, tempoVida, mensagem);

  return;
}

/*
 * Thread "viatura" responsável por enviar as informações da viatura gerada ao parque.
 */
void *gestaoEntrada(void * arg) {

    pthread_detach(pthread_self()); // Thread do tipo detached.

    clock_t tempoInicial = clock();
    clock_t ticks;

    infoViatura * info = (infoViatura *) arg;

    // Atribui as variaveis correspondentes o nome do fifo do controlador e do fifo privado da viatura.
    char pathname[7];
    sprintf(pathname, "fifo%c", info->portaAcesso);

    char fifoPrivado[20];
    sprintf(fifoPrivado, "fifo%d", info->id);

    if (mkfifo(fifoPrivado, PERMISSIONS) == -1) {
      perror(fifoPrivado);
      return NULL;
    }

    /*
     * Acesso ao fifo do controlador para escrever a viatura.
     * Início de zona crítica.
     */

    sem_wait(semaforo);

    int fd;
    // Caso não consiga abrir o fifo porque o parque já encerrou.
    if ((fd = open(pathname, O_WRONLY)) == -1) {
      sem_post(semaforo);
      unlink(fifoPrivado);
      free(info);
      return NULL;
    }

    // Verifica se consegue escrever no fifo.
    if (write(fd, info, sizeof(infoViatura)) == -1) {
      sem_post(semaforo);
      close(fd);
      unlink(fifoPrivado);
      free(info);
      return NULL;
    }
    close(fd);

    sem_post(semaforo);

    /*
     * Fim de zona crítica.
     */

    if ((fd = open(fifoPrivado, O_RDONLY)) == -1) {
      unlink(fifoPrivado);
      free(info);
      return NULL;
    }

    char mensagem[BUF_SIZE];

    // Recebe primeira mensagem do fifo privado.
    read(fd, mensagem, BUF_SIZE);

    if (strcmp(mensagem, "entrou") == 0) {

      ticks = clock() - ticksInicial;
      logGerador((int) ticks, info->id, info->portaAcesso, (int) info->tempoEstacionamento, -1, mensagem);

      // Espera até receber mensagem de saída pelo fifo privado.
      read(fd, mensagem, BUF_SIZE);

      ticks = clock() - ticksInicial;
      clock_t tempoVida = clock() - tempoInicial;
      logGerador((int) ticks, info->id, info->portaAcesso, (int) info->tempoEstacionamento, (int) tempoVida, mensagem);
    }

    else if (strcmp(mensagem, "cheio") == 0) {
      // Caso o parque esteja cehio, escreve no ficheiro de log.
      ticks = clock() - ticksInicial;
      logGerador((int) ticks, info->id, info->portaAcesso, (int) info->tempoEstacionamento, -1, mensagem);
    }

    close(fd);
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

    // Write log file header
    fprintf(parqueLog, "%-8s ; %-7s ; %-6s ; %-10s ;  %-6s  ; %s\n", "t(ticks)", "id_viat", "destin", "t_estacion", "t_vida", "observ");

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

            pthread_t entrada;
            pthread_create(&entrada, NULL, gestaoEntrada, (void *) viatura);

            anterior = fim;

            indiceProximo = rand() % 10;
            proximo = probabilidades[indiceProximo] * uniRelogio;
        }

    } while(fim - inicio < duracao);

    sem_close(semaforo);
    sem_unlink(SEMNAME);

    pthread_exit(0);
  }
