#pragma once

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#include "Actions.h"
#include "Network.h"

void HandlePing(DataFormat data, SOCKET ClientSocket, int &PacketID)
{
	char buf[512] = "";

	Actions action = action_ping_response;
	int packetID = 1;
	int packetCount = 1;

	memcpy(&buf[0], &data.Account, sizeof(__int64));
	memcpy(&buf[sizeof(__int64)], &action, sizeof(Actions));
	memcpy(&buf[sizeof(__int64) + sizeof(int)], &packetID, sizeof(int));
	memcpy(&buf[sizeof(__int64) + sizeof(int) + sizeof(int)], &packetCount, sizeof(int));

	send(ClientSocket, buf, sizeof(int) + sizeof(__int64) + sizeof(int) + sizeof(int), 0);
}

void HandleAuth(DataFormat data, SOCKET ClientSocket, std::vector<char> buf, int &PacketID)
{

	/*   */	

	data.Account = 1;


	if (1)
	{
		data.ActionID = action_auth_success;

		memcpy(&buf[0], &data.Account, sizeof(__int64));
		memcpy(&buf[sizeof(__int64)], &data.ActionID, sizeof(Actions));

		SendToClient(&buf[0], buf.size() - 1, ClientSocket, ++PacketID);
	}
	else
	{

	}

}