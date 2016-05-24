CC = gcc
CFLAGS = -g -pthread -lrt -Wall

all: bin/parque bin/gerador

bin/parque: parque.c
				$(CC) parque.c -o bin/parque $(CFLAGS)
bin/gerador: gerador.c
				$(CC) gerador.c -o bin/gerador $(CFLAGS)

clean:
			rm -f bin/parque bin/gerador
