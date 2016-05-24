CC = gcc
CFLAGS = -g -pthread -lrt -Wall

all: bin/parque bin/gerador

bin/parque: parque.c auxiliar.h
				$(CC) parque.c -o bin/parque $(CFLAGS)
bin/gerador: gerador.c auxiliar.h
				$(CC) gerador.c -o bin/gerador $(CFLAGS)

clean:
			rm -f bin/parque bin/gerador
