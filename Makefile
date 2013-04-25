.PHONY: clean default grind gdb

CXX = clang++
CXXFLAGS = -g -O0 -Wall -Wextra -Werror
LDFLAGS = -g
LDLIBS = -lpthread -lgdbm

default: LineClient.o Broadcast.o Server.o

# Clients
LineClient.o: Server.h

# Server
Broadcast.o:
Server.o: Broadcast.h

clean:
	-rm *.o
