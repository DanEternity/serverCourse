#undef UNICODE

#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <thread>
#include <vector>
#include "Util.h"
#include "dbModule.h"

#include "Logic.h"
#include "Actions.h"
#include "Handlers.h"
#include "Network.h"
// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
//#pragma comment (lib, "sqlite3.dll")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"


int ClientThread(SOCKET ClientSocket);
void HandleBuffer(std::vector<char> buf, SOCKET ClientSocket, int &PacketID, AccEnviroment &Env);
/*

int AddToQueue(std::vector<char> rBuff, std::vector<std::pair<int, std::vector<std::vector<char>>>> &bf, std::vector<std::pair<int, int>> &status);
int CheckBufferStatus(std::vector<std::pair<int, std::vector<std::vector<char>>>> &bf, std::vector<std::pair<int, int>> &status);
int ClearBuffer(std::vector<std::pair<int, std::vector<std::vector<char>>>> &bf, std::vector<std::pair<int, int>> &status);
*/
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

	initDB("main.db");

	//Test(); // test

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
			//return 1;
		}

		SOCKADDR_IN addr;
		int addrlen = sizeof(addr);

		// Accept a client socket
		ClientSocket = accept(ListenSocket, (SOCKADDR*)&addr, &addrlen);
		if (ClientSocket == INVALID_SOCKET) {
			printf("accept failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			//return 1;
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

	//db shutdown
	closeDB();

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
	
	int PacketID = 0;
	AccEnviroment Env;


	std::vector<std::pair<int, std::vector<std::vector<char>>>> que;
	std::vector<std::pair<int, int>> status;
	DWORD nonBlocking = 1;
	if (ioctlsocket(ClientSocket, FIONBIO, &nonBlocking) != 0)
	{
		printf("failed to set non-blocking socket\n");
		return false;
	}

	// Receive until the peer shuts down the connection
	do {

		iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0) {
			printf("Bytes received: %d\n", iResult);
			printf("%.*s\n", iResult, recvbuf);

			// Handle buffer
			if (iResult != 512)
				continue;
			
			std::vector<char> rBuff(512);
			memcpy(&rBuff[0], recvbuf, 512);

			AddToQueue(rBuff, que, status);

			//HandleBuffer(recvbuf, iResult, ClientSocket); // data / size / socket

		}
		else if (iResult == 0)
			printf("Connection closing...\n");
		else {
			auto err = WSAGetLastError();
			if (err == 10035)
			{

				int id = CheckBufferStatus(que, status);
				if (id != -1)
				{
					std::vector<char> buff;
					buff = BuffToRaw(que[id].second);

					HandleBuffer(buff, ClientSocket, PacketID, Env);

					
				}


				ClearBuffer(que, status);
				iResult = 1;
				continue;
			}
			printf("recv failed with error: %d\n", err);
			//closesocket(ClientSocket);
			//WSACleanup();
			//return 1;
		}

	} while (iResult > 0);
	
	// shutdown the connection since we're done
	iResult = shutdown(ClientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		//return 1;
	}

	// cleanup
	closesocket(ClientSocket);
	printf("Connection closed with %d\n", ClientSocket);
	return 0;
}



void HandleBuffer(std::vector<char> buf, SOCKET ClientSocket, int &PacketID, AccEnviroment &Env)
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

	/*  */
	/*
	int pacID = rand() % 2000000;
	SendToClient("sadasdasdasda", 14, ClientSocket, pacID);
	return;
	*/
	/*  */
	DataFormat dataHeader;

	if (buf.size() < sizeof(__int64) + sizeof(Actions) + sizeof(int) + sizeof(int))
	{
		printf("Error! Size of packet was %d\n", buf.size());

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
		HandlePing(dataHeader, ClientSocket, PacketID, Env);
		break;
	case action_auth:
		HandleAuth(dataHeader, ClientSocket, buf, PacketID, Env);
		break;
	case action_conf_user:
		HandleUserConferenceList(dataHeader, ClientSocket, PacketID, Env);
		break;
	case action_conf_full_info:
		HandleConferenceFullInfo(dataHeader, ClientSocket, buf, PacketID, Env);
		break;
	case action_get_conf_members:
		HandleConferenceMembers(dataHeader, ClientSocket, buf, PacketID, Env);
		break;
	case action_update_conf_info:
		HandleUpdateConfInfo(dataHeader, ClientSocket, buf, PacketID, Env);
		break;
	case action_leave_conf:
		HandleLeaveConf(dataHeader, ClientSocket, buf, PacketID, Env);
		break;
	case action_message_list:
		HandleMessageList(dataHeader, ClientSocket, buf, PacketID, Env);
		break;
	case action_get_message:
		HandleGetMessage(dataHeader, ClientSocket, buf, PacketID, Env);
		break;
	case action_search_conf:
		HandleSearcConf(dataHeader, ClientSocket, buf, PacketID, Env);
		break;
	case action_join_conf:
		HandleJoinConf(dataHeader, ClientSocket, buf, PacketID, Env);
		break;
	case action_send_message:
		HandleSendMessage(dataHeader, ClientSocket, buf, PacketID, Env);
		break;
	default:
		printf("Error! Cant handle %d action\n", &dataHeader.ActionID);
		// no such action5
		// error
		break;
	}
	


} 

