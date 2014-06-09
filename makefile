CC=g++
CFLAGS=-Wall

all: crts crts_rx

crts: crts.cpp
	$(CC) $(CFLAGS) crts.cpp -o crts -lm -lliquid -lpthread -lconfig -luhd -lliquidusrp

crts_rx: crts_rx.cpp
	$(CC) $(CFLAGS) crts_rx.cpp -o crts_rx -lm -lliquid -lpthread -lconfig -luhd -lliquidusrp

clean:
	rm crts crts_rx
