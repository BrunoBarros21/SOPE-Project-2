#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_ENTRADAS 4 

typedef struct{
  int numLugares;
  int tempoAbertura;  
}caracteristicas;

void *Test(void * arg){
    printf("Thread %d\n",* (int *) arg);
}

void *Parque(void * arg){
    pthread_t controladores[NUM_ENTRADAS];
    
    int i;
    for (i = 0; i < NUM_ENTRADAS; ++i){
        pthread_create(&controladores[i], NULL, Test, (void *) &i);
        pthread_join(controladores[i], NULL);
    }
}

int main(int argc, char* argv[]) {
    
    int numLugares;
    int tempoAbertura;
    
    if (argc != 3)
    {
        printf("Usage: %s <N_LUGARES> <T_ABERTURA>\n", argv[0]);
        exit(1);
    }
    
    //TODO Verificar valor retorno
    numLugares = (int) strtol(argv[1], NULL, 10);
    tempoAbertura = (int) strtol(argv[2], NULL, 10);
    
    caracteristicas parqueCar;
    parqueCar.numLugares = numLugares;
    parqueCar.tempoAbertura = tempoAbertura;
    
    pthread_t parque;
    pthread_create(&parque, NULL, Parque, (void *) &parqueCar);
    
    pthread_exit(0);
}
