LFLAG=-pthread
CFLAG=-Wall -Wextra -pedantic
client=Client
server=Server
CC=g++
RM=/bin/rm

client: Client.o
	$(CC) -o $(client) Client.o $(LFLAG)

server: Server.o
	$(CC) -o $(server) Server.o $(LFLAG)

%.o: %.cpp
	$(CC) $(CFLAG) -c -o $@ $<
clean:
	$(RM) -f Client.o Server.o Server Client
