#include <iostream>
#include <winsock.h>
#include <string>
using namespace std;

//Server Settings ---------------------------------
#define PORT 3939
#define bufferSize 768 //Hanya dipakai dalam menerima data Client
#define maxClients 100
//-------------------------------------------------

struct sockaddr_in srv;
fd_set fr, fw, fe;
int nMaxFd, nSocket, nArrClient[maxClients];
string clientsID[maxClients];

void receiveMessage(int nClientSocket) {
	char buff[bufferSize + 1] = { 0, };
	int nRet = recv(nClientSocket, buff, bufferSize, 0);

	//-------------------------------------------------
	//Client disconnects ------------------------------
	if (nRet < 0) {
		closesocket(nClientSocket);
		string cID;
		for (int i = 0; i < maxClients; i++) {
			if (nArrClient[i] == nClientSocket) {
				cID = clientsID[i];
				break;
			}
		}
		cout << "[INFO] Client ID " << cID << " (Socket " << nClientSocket << ") was disconnected.";
		for (int i = 0; i < maxClients; i++) {
			if (nArrClient[i] == nClientSocket) {
				nArrClient[i] = 0;
				clientsID[i] = "0";
				break;
			}
		}
	}
	//Receive commands from Clients -------------------
	else {
		//0001/Spec/CPU/Intel Core i7/
		//0001/Status/Shutdown/09-03-2023 03:39/
		//0001/Uptime/0:03:39:39/
		//0001/Apps/App A/App B/App C/ .dst/

		string cID;
		for (int i = 0; i < maxClients; i++) {
			if (nArrClient[i] == nClientSocket) {
				cID = clientsID[i];
				break;
			}
		}

		int n = 0, prev = 0, nApps = 0; bool isCommand = false;
		string text(buff), id, command, status, spec, apps[300], value; //( Cek ide dibawah )
		if (!text.empty()) {
			for (int i = 0; i < text.length(); i++) {
				if (text[i] == '/') {
					if (n == 0) {
						id = text.substr(prev, (i - prev));
					}
					else if (n == 1) {
						command = text.substr(prev, (i - prev));
					}
					else if (n > 1) {
						cout << "[INFO] Receive [" << command << "] command from Client ID " << cID << " (Socket" << nClientSocket << ").\n";
						if (command == "Status") {
							isCommand = true;
							if (status.empty()) {
								status = text.substr(prev, (i - prev));
							}
							else {
								value = text.substr(prev, (i - prev));
							}
						}
						else if (command == "Spec") {
							isCommand = true;
							if (spec.empty()) {
								spec = text.substr(prev, (i - prev));
							}
							else {
								value = text.substr(prev, (i - prev));
							}
						}
						else if (command == "Apps") {
							isCommand = true;
							apps[nApps] = text.substr(prev, (i - prev));
							nApps++;
						}
						else if (command == "Uptime") {
							isCommand = true;
							value = text.substr(prev, (i - prev));
						}
					}
					n++;
					prev = i + 1;
				}
			}

			//( Lakukan sesuatu dengan variable yang sudah didapatkan)
			//INFO: Maks buffer untuk kirim pesan ke Client adalah 30.
			//IDE: Mending terima command langsung diproses daripada ditaruh dalam variable.
			//     Variable string nya dikurangi jadi commands & value saja (ID dari array clientId).
			//     Nanti dalam "else if command == ....", baru dibaca value nya, di-substring, di proses.

		}

		if (!isCommand) {
			cout << "[WARN] Invalid command from Client ID " << cID << " (Socket " << nClientSocket << ").\n";
		}
	}
	//-------------------------------------------------
}

void receiveNewConnection() {
	char buff[bufferSize + 1] = { 0, };
	if (FD_ISSET(nSocket, &fr)) {
		int nLen = sizeof(struct sockaddr);
		int nClientSocket = accept(nSocket, NULL, &nLen);

		if (nClientSocket > 0) {
			int nIndex;
			for (nIndex = 0; nIndex < maxClients; nIndex++) {
				if (nArrClient[nIndex] == 0) {
					nArrClient[nIndex] = nClientSocket;
					recv(nClientSocket, buff, 10, 0);
					string newId(buff);
					clientsID[nIndex] = newId;
					cout << "[INFO] Found Client ID " << newId << " (Socket " << nClientSocket << ")\n";

					//DATABASE:
					//Cek ID dengan database, jika tidak ada, 
					//buat ID baru (Auto-increment).

					break;
				}
			}
			if (nIndex >= maxClients) {
				cout << "[WARN] Connection is full!\n";
			}
		}
	}
	else {
		for (int i = 0; i < maxClients; i++) {
			if (FD_ISSET(nArrClient[i], &fr)) {
				receiveMessage(nArrClient[i]);
			}
		}
	}
}

void exitFail() {
	WSACleanup();
	system("PAUSE");
	exit(EXIT_FAILURE);
}

int main() {
	int nRet = 0;
	cout << "-------------------------------------\n";
	cout << "[ Server settings ]\n\n";
	cout << "Server IP: " << INADDR_ANY << endl;
	cout << "Connection Port: " << PORT << endl;
	cout << "Maximum Clients: " << maxClients << endl;
	cout << "Buffer size: " << bufferSize << endl;
	cout << "-------------------------------------\n\n";

	//-------------------------------------------------
	//1. Initialize WSA -------------------------------
	WSADATA ws;
	if (WSAStartup(MAKEWORD(2, 2), &ws) < 0) {
		cout << "[ERROR] Error initializing WSA.\n";
		exitFail();
	}
	//2. Create socket --------------------------------
	nSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (nSocket < 0) {
		cout << "[ERROR] Error creating socket.\n";
		exitFail();
	}
	//3. Enviroment Settings --------------------------
	srv.sin_family = AF_INET;
	srv.sin_port = htons(PORT);
	srv.sin_addr.s_addr = INADDR_ANY; //Get local Machine IP
	memset(&(srv.sin_zero), 0, 8);
	//4. Setsockopt -----------------------------------
	int nOptVal = 0; int nOptLen = sizeof(nOptVal);
	nRet = setsockopt(nSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&nOptVal, nOptLen);
	if (nRet) {
		cout << "[ERROR] Error during Setsockopt.\n";
		exitFail();
	}
	//5. Bind -----------------------------------------
	nRet = bind(nSocket, (sockaddr*)&srv, sizeof(sockaddr));
	if (nRet < 0) {
		cout << "[ERROR] Error binding to Local port.\n";
		exitFail();
	}

	//6. Listens to local port ------------------------
	nRet = listen(nSocket, maxClients);
	if (nRet < 0) {
		cout << "[ERROR] Error listening to Local port.\n";
		exitFail();
	}
	//-------------------------------------------------

	nMaxFd = nSocket;
	struct timeval tv;
	tv.tv_sec = 1; tv.tv_usec = 0; //Wait new request every second.
	cout << "[INFO] Server is running...\n";
	while (1) {
		//-------------------------------------------------
		//Clear socket descriptors ------------------------
		FD_ZERO(&fr);
		FD_ZERO(&fw);
		FD_ZERO(&fe);
		//Set socket descriptors --------------------------
		FD_SET(nSocket, &fr);
		FD_SET(nSocket, &fe);
		//Send & Receive message with client. -------------
		for (int i = 0; i < maxClients; i++) {
			if (nArrClient[i] != 0) {
				FD_SET(nArrClient[i], &fr);
				FD_SET(nArrClient[i], &fe);
			}
		}
		//-------------------------------------------------

		//Listening to new requests from Clients ----------
		nRet = select(nMaxFd + 1, &fr, &fw, &fe, &tv);
		if (nRet > 0) { receiveNewConnection(); }
		else if (nRet == 0) {}
		else {
			cout << "[ERROR] Failed to process requess." << endl;
			WSACleanup();
			exit(EXIT_FAILURE);
		}
		//-------------------------------------------------
	}
}