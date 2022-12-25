CC = gcc
AR=ar
FLAGS = -Wall -fPIC -g

all: Receiver Sender
Receiver: Receiver.c
	$(CC) $(FLAGS) -o Receiver Receiver.c

Sender: Sender.c
	$(CC) $(FLAGS) -o Sender Sender.c
clean:
	rm -f Receiver Sender *.o


.PHONY: clean all Sender Receiver