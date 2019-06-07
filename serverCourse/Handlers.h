#pragma once

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#include "Actions.h"
#include "Network.h"
#include "dbModule.h"

void HandlePing(DataFormat data, SOCKET ClientSocket, int &PacketID)
{
	char buf[512] = "";

	Actions action = action_ping_response;
	int packetID = 1;
	int packetCount = 1;

	memcpy(&buf[0], &data.Account, sizeof(__int64));
	memcpy(&buf[sizeof(__int64)], &action, sizeof(Actions));
	memcpy(&buf[sizeof(__int64) + sizeof(Actions)], &packetID, sizeof(int));
	memcpy(&buf[sizeof(__int64) + sizeof(Actions) + sizeof(int)], &packetCount, sizeof(int));

	send(ClientSocket, buf, sizeof(int) + sizeof(__int64) + sizeof(int) + sizeof(int), 0);
}

void HandleAuth(DataFormat data, SOCKET ClientSocket, std::vector<char> buf, int &PacketID)
{

	int login_start;
	int login_size;
	int password_size;

	int off_q = sizeof(__int64) + sizeof(Actions) + sizeof(int) + sizeof(int);
	memcpy(&login_start, &buf[0 + off_q], sizeof(int));
	memcpy(&login_size, &buf[0 + off_q + sizeof(int)], sizeof(int));
	memcpy(&password_size, &buf[0 + off_q + sizeof(int) + sizeof(int)], sizeof(int));

	std::vector<char> t_login(login_size, 0);
	std::vector<char> t_password(password_size, 0);

	memcpy(&t_login[0], &buf[login_start + off_q], sizeof(char)*login_size);
	memcpy(&t_password[0], &buf[login_start + login_size + off_q], sizeof(char)*password_size);

	/*   */	

	std::string sqlText = "SELECT * FROM Account WHERE Login = \'";
	for (char w : t_login)
		sqlText += w;
	sqlText += "\' AND Password = \'";
	for (char w : t_password)
		sqlText += w;
	sqlText += "\'";
	ExecSQL(sqlText, DBCallbackFunc_Account);

	

	//data.Account = ;


	if (!resultBuffer.empty())
	{

		ContentAccount * a = static_cast<ContentAccount*>(resultBuffer[0]);
		data.Account = a->id;
		data.ActionID = action_auth_success;
		
		memcpy(&buf[0], &data.Account, sizeof(__int64));
		memcpy(&buf[sizeof(__int64)], &data.ActionID, sizeof(Actions));
		memcpy(&buf[0 + off_q], &a->accType, sizeof(int));
		SendToClient(&buf[0], buf.size(), ClientSocket, ++PacketID);
		delete a;
		resultBuffer.pop_back();
	}
	else
	{
		data.ActionID = action_auth_fail;

		//memcpy(&buf[0], &data.Account, sizeof(__int64));
		memcpy(&buf[sizeof(__int64)], &data.ActionID, sizeof(Actions));

		SendToClient(&buf[0], buf.size(), ClientSocket, ++PacketID);
	}

}

void HandleUserConferenceList(DataFormat data, SOCKET ClientSocket, int &PacketID)
{

	int id = data.Account;

	std::string sqlText = "SELECT * FROM Conference, Participation WHERE Conference.id = Participation.idConference AND Participation.idScientist = \'";
	char c[20] = "";
	_itoa_s(id, c, 10);
	int i = 0;
	while (c[i] != 0) sqlText += c[i++];
	sqlText += "\'";
	ExecSQL(sqlText, DBCallbackFunc_Conference);

	std::vector<char> buf(30);
	Actions action = action_conf_user_responce;
	memcpy(&buf[0], &data.Account, sizeof(__int64));
	memcpy(&buf[sizeof(__int64)], &action, sizeof(Actions));
	//memcpy(&buf[sizeof(__int64) + sizeof(Actions)], &PacketID, sizeof(int));
	//memcpy(&buf[sizeof(__int64) + sizeof(Actions) + sizeof(int)], &packetCount, sizeof(int));
	int sz = resultBuffer.size();
	memcpy(&buf[20U], &sz, sizeof(int));
	int offset = 20U + sizeof(int);
	for (int i(0); i < resultBuffer.size(); i++)
	{
		ContentConference * q = static_cast<ContentConference*>(resultBuffer[i]);
		buf.resize(buf.size() + 8 + q->name.size());
		memcpy(&buf[offset], &q->id, sizeof(int));
		int ssz = q->name.size();
		memcpy(&buf[offset + sizeof(int)], &ssz, sizeof(int));
		memcpy(&buf[offset + sizeof(int) + sizeof(int)], q->name.c_str(), sizeof(char) * ssz);
		offset += 8 + ssz;
		delete q;
	}

	SendToClient(&buf[0], buf.size(), ClientSocket, PacketID);

	resultBuffer.clear();
}