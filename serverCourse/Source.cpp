#undef UNICODE

#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <thread>

#include "Actions.h"
#include "Handlers.h"

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

int ClientThread(SOCKET ClientSocket);
void HandleBuffer(char * buf, int size, SOCKET ClientSocket);

int __cdecl main(void)
{
	WSADATA wsaData;
	int iResult;

	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;

	struct addrinfo *result = NULL;
	struct addrinfo hints;

	int iSendResult;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result);
	printf("server started\nlistening connections...\n");
	while (1)
	{
		iResult = listen(ListenSocket, SOMAXCONN);
		if (iResult == SOCKET_ERROR) {
			printf("listen failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}

		SOCKADDR_IN addr;
		int addrlen = sizeof(addr);

		// Accept a client socket
		ClientSocket = accept(ListenSocket, (SOCKADDR*)&addr, &addrlen);
		if (ClientSocket == INVALID_SOCKET) {
			printf("accept failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}

		char * client_ip = inet_ntoa(addr.sin_addr);
		unsigned short client_port = ntohs(addr.sin_port);
		printf("Accepted Connection from :  %s port %d\n", client_ip, client_port);

		std::thread * thr = new std::thread(ClientThread, ClientSocket);
		thr->detach();
	}
	// No longer need server socket
	closesocket(ListenSocket);

	// cleanup
	WSACleanup();

	system("PAUSE");
	return 0;
}

int ClientThread(SOCKET ClientSocket)
{
	int iResult;
	struct addrinfo *result = NULL;
	struct addrinfo hints;

	int iSendResult;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;

	// Receive until the peer shuts down the connection
	do {

		iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0) {
			printf("Bytes received: %d\n", iResult);
			printf("%.*s\n", iResult, recvbuf);

			// Handle buffer

			HandleBuffer(recvbuf, iResult, ClientSocket); // data / size / socket

		}
		else if (iResult == 0)
			printf("Connection closing...\n");
		else {
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(ClientSocket);
			WSACleanup();
			return 1;
		}

	} while (iResult > 0);

	// shutdown the connection since we're done
	iResult = shutdown(ClientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		return 1;
	}

	// cleanup
	closesocket(ClientSocket);
	return 0;
}

void HandleBuffer(char * buf, int size, SOCKET ClientSocket)
{
	/* Buffer format */
	/*
		Account 
		ActionID
		PacketID
		PacketCountExpected
		ParametersCount
		<params>
	*/

	DataFormat dataHeader;

	if (size < sizeof(__int64) + sizeof(Actions) + sizeof(int) + sizeof(int))
	{
		printf("Error! Size of packet was %d\n", size);

		// need to answer with @ErrorResponse@

		return;
	}
	
	dataHeader.Account = (__int64)buf[0];
	dataHeader.ActionID = (Actions)buf[sizeof(__int64)];
	dataHeader.PacketID = (int)buf[sizeof(__int64) + sizeof(Actions)];
	dataHeader.PacketCountExpected = (int)buf[sizeof(__int64) + sizeof(Actions) + sizeof(int)];

	switch (dataHeader.ActionID)
	{
	case action_ping:
		HandlePing(dataHeader, ClientSocket);
		break;
	default:
		// no such action
		// error
		break;
	}



}