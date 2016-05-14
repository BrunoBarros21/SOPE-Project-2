#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct{
    char portaAcesso;
    clock_t tempoEstacionameto;
} infoViatura;

void *gestaoEntrada(void * arg){
    infoViatura info = *(infoViatura *) arg;
    
    printf ("Entrada: %c\nTempo Estacionamento: %f\n\n", info.portaAcesso, (float)info.tempoEstacionameto / CLOCKS_PER_SEC);
    
    pthread_exit(0);
}

int main (int argc, char* argv[]) {
    int tempoGeracao;
    int uniRelogio;
    char acessos[4] = {'N', 'E', 'S', 'O'}; 
    int probabilidades[10] = {0, 0, 0, 0, 0, 1, 1, 1, 2, 2};

    if (argc != 3)
    {
        printf("Usage: %s <T_GERACAO> <U_RELOGIO>\n", argv[0]);
        exit(1);
    }

    tempoGeracao = (int) strtol(argv[1], NULL, 10);
    uniRelogio = (int) strtol(argv[2], NULL, 10);
    
    srand(time(NULL));
    
    clock_t inicio = clock();
    clock_t fim;
    clock_t duracao = tempoGeracao * CLOCKS_PER_SEC;
    
    
    do{
        fim = clock();
        
        int indiceProximo = rand() % 10;
        int proximo = probabilidades[indiceProximo];
        
        if((fim - inicio) > proximo * uniRelogio){
            int indiceAcesso = rand() % 4;
            int tempoEstacionameto = rand() % 10 + 1;
            
            infoViatura viatura;
            viatura.portaAcesso =  acessos[indiceAcesso];
            viatura.tempoEstacionameto = tempoEstacionameto * uniRelogio;
            
            pthread_t entrada;
            
            pthread_create(&entrada, NULL, gestaoEntrada, (void *) &viatura);
        }
        
        /*if(((float)(fim - inicio)/ CLOCKS_PER_SEC) - ((int)(fim - inicio)/CLOCKS_PER_SEC) == 0.0)
            printf("Tempo: %ld\n", (int)(fim - inicio) / CLOCKS_PER_SEC);*/
        
    }while(fim - inicio < duracao);
    
    return 0;
  }
