CC=gcc
AR=ar
FLAGS= -Wall -g


all: sender Measure

sender: sender.o
	$(CC) $(FLAGS) sender.c -o sender

Measure: Measure.o
	$(CC) $(FLAGS) Measure.c -o Measure


.PHONY: clean all

clean: # Remove any file that might created.
	rm -f *.o *.a *.so *.gch sender Measure