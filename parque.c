#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>

#define N 0
#define E 1
#define S 2
#define O 3
#define NUM_ENTRADAS 4

#define FIFOPER S_IRUSR | S_IWUSR

int numLugares;

typedef struct {
    int id;
    char portaAcesso;
    clock_t tempoEstacionameto;
} infoViatura;

typedef struct {
  int numLugares;
  int tempoAbertura;
} caracteristicas;

char *getFIFOPath (char ori) {
  char* pathname = malloc(strlen("./fifo" + 2));

  strncpy(pathname, "./fifo", sizeof("./fifo"));
  strncat(pathname, &ori, 1);

  return pathname;
}

void *Arrumador(void * arg) {
    infoViatura infoCarro = * (infoViatura *) arg;
    
    if (numLugares > 0)
        numLugares--;
    
    else 
    {
        char* pathname = malloc(strlen("./fifo" + 3));
        sprintf(pathname, "./fifo%d", infoCarro.id);
        
        int fd = open(pathname, O_WRONLY);
        char mensagem[32];
        sprintf(mensagem, "Carro nao tem lugar disponivel\n");
        write(fd, mensagem, sizeof(mensagem));
        close(fd);
    }
    
    pthread_exit(0);
}

void *Controlador (void * arg) {
    char ori = * (int *) arg;
    char* pathname = getFIFOPath(ori);
    printf("Thread %d\n", ori);

    if (mkfifo(pathname, FIFOPER) == -1) {
      perror(pathname);
    }

    int fd = open(pathname, O_RDONLY);

    char teste[256];
    int r;

    while ((r=read(fd, teste, 256)) >= 0) {
        if (r != 0)
            printf("%s\n", teste);
        if (strncmp(teste, "ACABOU", 6) == 0) {
            printf("%c\n", ori);
            break;
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

    caracteristicas parqueCar;
    parqueCar.numLugares = numLugares;
    parqueCar.tempoAbertura = tempoAbertura;

    pthread_t controladores[NUM_ENTRADAS];

    int i;
    for (i = 0; i < NUM_ENTRADAS; ++i) {
        pthread_create(&controladores[i], NULL, Controlador, (void *) &acessos[i]);
        //pthread_join(controladores[i], NULL);
    }

    sleep(tempoAbertura);

    for (i = 0; i < NUM_ENTRADAS; ++i) {
      int fd = open(getFIFOPath(acessos[i]), O_WRONLY);

      write(fd, "ACABOU", sizeof("ACABOU"));
      close(fd);
    }


    pthread_exit(0);
}
