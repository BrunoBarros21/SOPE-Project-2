#include <stdio.h>
#include <stdlib.h>

int main (int argc, char* argv[]) {
    int tempoGeracao;
    int uniRelogio;

    if (argc != 3)
    {
        printf("Usage: %s <T_GERACAO> <U_RELOGIO>\n", argv[0]);
        exit(1);
    }

    tempoGeracao = (int) strtol(argv[1], NULL, 10);
    uniRelogio = (int) strtol(argv[2], NULL, 10);

    return 0;
  }
