# makefile

all: dataserver client

reqchannel.o: reqchannel.h reqchannel.cpp
	g++ -std=c++11 -c -g reqchannel.cpp

BoundedBuffer.o: BoundedBuffer.h BoundedBuffer.cpp semaphore.h
	g++ -std=c++11 -c -g BoundedBuffer.cpp

dataserver: dataserver.cpp reqchannel.o 
	g++ -std=c++11 -g -o dataserver dataserver.cpp reqchannel.o -lpthread

client: client.cpp reqchannel.o BoundedBuffer.o semaphore.h
	g++ -std=c++11 -g -o client client.cpp reqchannel.o BoundedBuffer.o -lpthread -w
clean:
	rm -rf *.o fifo* dataserver client
