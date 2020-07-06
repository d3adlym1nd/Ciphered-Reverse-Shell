LFLAG=-pthread -lws2_32
CFLAG=-Wall -Wextra -pedantic -g
client=Client.exe
server=Server.exe
CC=g++
RM=cmd.exe /c del

client: Client.o
	$(CC) -o $(client) Client.o $(LFLAG)

server: Server.o
	$(CC) -o $(server) Server.o $(LFLAG)

%.o: %.cpp
	$(CC) $(CFLAG) -c -o $@ $<
clean:
	$(RM) Client.o Server.o Server.exe Client.exe
