CC=gcc

all: feed manager


feed.o : feed.c util.h
	gcc feed.c -c


feed : feed.o
	gcc feed.o -o feed


manager.o : manager.c util.h
	gcc manager.c -c

manager: manager.o
	gcc manager.o -o manager

clear:
	rm *.o
