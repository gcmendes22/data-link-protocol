CC = gcc
CFLAGS = -Wall

all: data-link-protocol

data-link-protocol: app.o linklayer.o
	gcc -o data-link-protocol app.o linklayer.o

app.o: app.c linklayer.h	
	gcc -o app.o app.c -c -Wall

linklayer.o: linklayer.c linklayer.h
	gcc -o linklayer.o linklayer.c -c -Wall

clean:
	rm -rf *.o *~ data-link-protocol