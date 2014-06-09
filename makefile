CC=g++
CFLAGS=-Wall -g

all: crts 

crts: crts.cpp
	$(CC) $(CFLAGS) crts.cpp -o crts -lm -lliquid -lpthread -lconfig -luhd -lliquidusrp

clean:
	rm crts 
