CC = gcc

CFLAGS = -Wall -g

OBJS = driver.o

all: flap

flap: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) -lncurses

clean: 
	rm -f *.o *~ flap

.PHONY: all clean
