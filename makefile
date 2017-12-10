CC=g++
CFLAGS=-I
EXEC = client server cord
all: $(EXEC)

client: client.cpp
	$(CC) -o client client.cpp
server: server.cpp
	$(CC) -o server server.cpp -lpthread
cord: coordinator.cpp
	$(CC) -o cord coordinator.cpp -lpthread
clean:
	rm $(EXEC)
