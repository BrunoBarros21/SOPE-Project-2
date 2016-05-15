#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>

#define FIFOPER S_IRUSR | S_IWUSR

unsigned int idViatura;

typedef struct {
    int id;
    char portaAcesso;
    clock_t tempoEstacionameto;
} infoViatura;

void *gestaoEntrada(void * arg){
    pthread_detach(pthread_self());
    infoViatura info = *(infoViatura *) arg;
    char teste[256];
    sprintf(teste, "Entrada: %c\nTempo Estacionamento: %f\nID: %d\n\n", info.portaAcesso, (float)info.tempoEstacionameto / CLOCKS_PER_SEC, info.id);

    char pathname[7];
    sprintf(pathname, "./fifo%c", info.portaAcesso);

    char fifoPrivado[20];
    sprintf(fifoPrivado, "./fifo%d", info.id);

    if (mkfifo(fifoPrivado, FIFOPER) == -1) {
      perror(fifoPrivado);
    }

    int fd = open(pathname, O_WRONLY);
    write(fd, teste, sizeof(teste));
    close(fd);

    pthread_exit(0);
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

    srand(time(NULL));

    clock_t duracao = tempoGeracao * CLOCKS_PER_SEC;
    clock_t fim;
    clock_t inicio = clock();
    clock_t anterior = inicio;

    int indiceProximo;
    int proximo = 0;

    do {
        fim = clock();

        if((fim - anterior) >= proximo) {
            int indiceAcesso = rand() % 4;
            int tempoEstacionameto = rand() % 10 + 1;

            infoViatura *viatura = malloc(sizeof(viatura));
            viatura->portaAcesso = acessos[indiceAcesso];
            viatura->tempoEstacionameto = tempoEstacionameto * uniRelogio;
            viatura->id = idViatura++;

            printf("%d\n", viatura->id);

            pthread_t entrada;
            pthread_create(&entrada, NULL, gestaoEntrada, (void *) viatura);

            anterior = fim;

            indiceProximo = rand() % 10;
            proximo = probabilidades[indiceProximo] * uniRelogio;

            printf("%d\n", proximo);
        }

        /*if(((float)(fim - inicio)/ CLOCKS_PER_SEC) - ((int)(fim - inicio)/CLOCKS_PER_SEC) == 0.0)
            printf("Tempo: %ld\n", (int)(fim - inicio) / CLOCKS_PER_SEC);*/
    } while(fim - inicio < duracao);

    return 0;
  }
