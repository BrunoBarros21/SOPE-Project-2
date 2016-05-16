CC = gcc
CFLAGS = -Wall

all: bin/parque bin/gerador

bin/parque: parque.c
				$(CC) parque.c -pthread -o bin/parque $(CFLAGS)
bin/gerador: gerador.c
				$(CC) gerador.c -pthread -o bin/gerador $(CFLAGS)

clean:
			rm -f bin/parque bin/gerador
