#include <iostream>
#include <winsock.h>
#include <string>
using namespace std;
#define PORT 9909
#define maxBuffer 768
#define maxClients 100

struct sockaddr_in srv;
fd_set fr, fw, fe;
int nMaxFd, nSocket, nArrClient[maxClients];

void receiveMessage(int nClientSocket) {
	char buff[maxBuffer + 1] = { 0, };
	int nRet = recv(nClientSocket, buff, maxBuffer, 0);
	if (nRet < 0) {
		cout << "Closing connection from socket " << nClientSocket << endl;
		closesocket(nClientSocket);
		for (int i = 0; i < maxClients; i++) {
			if (nArrClient[i] == nClientSocket) {
				nArrClient[i] = 0;
				break;
			}
		}
	}
	else {
		cout << "Client socket " << nClientSocket << " says: " << buff << endl;

		//Contoh value:
		//0001/Spec/CPU/Intel Core i7/
		//0001/Status/Shutdown/09-03-2023 03:39/
		//0001/Uptime/0:03:39:39/
		//0001/Apps/App A/App B/App C/ .dst/

		int n = 0, prev = 0, nApps = 0;
		bool isCommand = false;
		string text(buff), id, command, status, spec, apps[300], value;
		if (!text.empty() && text != ".") {
			for (int i = 0; i < text.length(); i++) {
				if (text[i] == '/') {
					if (n == 0) {
						id = text.substr(prev, (i - prev));
					}
					else if (n == 1) {
						command = text.substr(prev, (i - prev));
					}
					else if (n > 1) {
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

			//DEBUG
			//cout << "\nID: " << id << ", Command: " << command << ", Status: " << status << ", Spec: " << spec << ", Value: " << value;
			//cout << "\nApps:\n";
			//for (int i = 0; i < nApps; i++) {
			//	cout << apps[i] << ", ";
			//}
		}

		if (!isCommand /*&& text != "." */) {
			cout << "\n( Invalid command )\n";
			send(nClientSocket, "Error.", maxBuffer, 0);
		}
		else {
			send(nClientSocket, "Success.", maxBuffer, 0);
		}
		cout << endl;
	}
}

void receiveNewConnection() {
	char buff[maxBuffer + 1] = { 0, };
	if (FD_ISSET(nSocket, &fr)) {
		int nLen = sizeof(struct sockaddr);
		int nClientSocket = accept(nSocket, NULL, &nLen);

		if (nClientSocket > 0) {
			int nIndex;
			for (nIndex = 0; nIndex < maxClients; nIndex++) {
				if (nArrClient[nIndex] == 0) {
					nArrClient[nIndex] = nClientSocket;

					////DEBUG
					//cout << "nArrClient:\n";
					//for (int i = 0; i < maxClients; i++) {
					//	cout << nArrClient[i] << " ";
					//}
					//cout << "\nnArrClientID:\n";
					//for (int i = 0; i < maxClients; i++) {
					//	cout << nArrClientID[i] << " ";
					//}
					//cout << endl;

					cout << "A new client has just connected!\n";
					break;
				}
			}
			if (nIndex >= maxClients) {
				cout << "Current connection is full!\n";
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

int main()
{
	int nRet = 0;

	//Initialize WSA
	WSADATA ws;
	if (WSAStartup(MAKEWORD(2, 2), &ws) < 0) {
		cout << "Failed initialize WSA.\n";
		WSACleanup();
		exit(EXIT_FAILURE);
	}

	//Open the socket
	nSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (nSocket < 0) {
		cout << "Socket isn't openned.\n";
		WSACleanup();
		exit(EXIT_FAILURE);
	}

	//Enviroment
	srv.sin_family = AF_INET;
	srv.sin_port = htons(PORT);
	srv.sin_addr.s_addr = INADDR_ANY; //Local Machine IP
	memset(&(srv.sin_zero), 0, 8);

	//Setsockopt
	int nOptVal = 0; int nOptLen = sizeof(nOptVal);
	nRet = setsockopt(nSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&nOptVal, nOptLen);
	if (nRet) {
		cout << "Setsockopt Failed.\n";
		WSACleanup();
		exit(EXIT_FAILURE);
	}

	//Bind
	nRet = bind(nSocket, (sockaddr*)&srv, sizeof(sockaddr));
	if (nRet < 0) {
		cout << "Failed binding to Local port.\n";
		WSACleanup();
		exit(EXIT_FAILURE);
	}

	//Listens to local port
	nRet = listen(nSocket, maxClients);
	if (nRet < 0) {
		cout << "Failed to listen to Local port.\n";
		WSACleanup();
		exit(EXIT_FAILURE);
	}

	nMaxFd = nSocket;
	struct timeval tv;
	tv.tv_sec = 1; tv.tv_usec = 0; //Wait new request every second.
	cout << "Server is running..\n";
	while (1) {
		//Clear socket descriptors
		FD_ZERO(&fr);
		FD_ZERO(&fw);
		FD_ZERO(&fe);

		//Set socket descriptors
		FD_SET(nSocket, &fr);
		//FD_SET(nSocket, &fw);
		FD_SET(nSocket, &fe);

		//Send & Receive message with client.
		for (int i = 0; i < maxClients; i++) {
			if (nArrClient[i] != 0) {
				FD_SET(nArrClient[i], &fr);
				FD_SET(nArrClient[i], &fe);
			}
		}

		//Waiting for new request & process those requests.
		nRet = select(nMaxFd + 1, &fr, &fw, &fe, &tv);
		if (nRet > 0) {
			cout << "Processing client request..\n";
			receiveNewConnection();
		}
		else if (nRet == 0) {
			//No connection / communication request has been made.
		}
		else {
			//Failed.
			cout << "Failed to process requess." << endl;
			WSACleanup();
			exit(EXIT_FAILURE);
		}
	}
}