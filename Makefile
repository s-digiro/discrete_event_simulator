CC = gcc
CFLAGS = -g -Wall

default: main

main: source/main.c source/queue.o source/min_heap.o
	$(CC) $(CFLAGS) -o main source/main.c source/queue.o source/min_heap.o -lm

queue.o: queue.c
	$(CC) $(CFLAGS) -o source/queue.o -c queue.c

min_heap.o: min_heap.c
	$(CC) $(CFLAGS) -o source/min_heap.o -c min_heap.c

clean:
	rm main source/*.o
