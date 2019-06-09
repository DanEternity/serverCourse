#pragma once

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#include "Actions.h"
#include "Network.h"
#include "dbModule.h"
#include "BufferHelp.h"

void HandlePing(DataFormat data, SOCKET ClientSocket, int &PacketID, AccEnviroment &Env)
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

void HandleAuth(DataFormat data, SOCKET ClientSocket, std::vector<char> buf, int &PacketID, AccEnviroment &Env)
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
		/* Env setup */
		Env.id = a->id;
		Env.accType = a->accType;

		/* */
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

void HandleUserConferenceList(DataFormat data, SOCKET ClientSocket, int &PacketID, AccEnviroment &Env)
{

	int id = data.Account;

	//std::string 
	std::string sqlText;
	if (Env.accType == 1)
		sqlText = "SELECT Conference.id, Conference.Name FROM Conference, Participation WHERE Conference.id = Participation.idConference AND Participation.idScientist = \'";
	else
		sqlText = "SELECT Conference.id, Conference.Name FROM Conference, Admin WHERE Conference.Admin = Admin.id AND Admin.Account = \'";

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

void HandleConferenceFullInfo(DataFormat data, SOCKET ClientSocket, std::vector<char> buf, int &PacketID, AccEnviroment &Env)
{
	std::string sqlText = "";
	sqlText += "SELECT Conference.id, Conference.Name, ";
	sqlText += "Conference.Date, Conference.Theme, ";
	sqlText += "Conference.City, Conference.Description, ";
	sqlText += "Conference.Admin, City.Name as CName, ";
	sqlText += "Theme.Name as TName ";
	sqlText += "FROM Conference, City, Theme ";
	sqlText += "WHERE Conference.Theme = Theme.id AND ";
	sqlText += "Conference.City = City.id AND ";
	sqlText += "Conference.id = \'";

	int confId;
	memcpy(&confId, &buf[20], sizeof(int));

	char t[20] = "";
	_itoa_s(confId, t, 10);

	for (int i(0); i < strlen(t); i++) sqlText += t[i];
	sqlText += "\'";
	
	printf("debug: ");
	printf("%s\n", sqlText.c_str());

	ExecSQL(sqlText, DBCallbackFunc_ConferenceFull);
	if (resultBuffer.empty())
	{
		data.ActionID = action_error;
		memcpy(&buf[0], &data.Account, sizeof(__int64));
		memcpy(&buf[sizeof(__int64)], &data.ActionID, sizeof(Actions));
		SendToClient(&buf[0], 24, ClientSocket, PacketID);
		return;
	}

	buf.clear();
	buf.assign(24, 0);
	data.ActionID = action_conf_full_info_response;
	memcpy(&buf[0], &data.Account, sizeof(__int64));
	memcpy(&buf[sizeof(__int64)], &data.ActionID, sizeof(Actions));
	int sz = resultBuffer.size();
	memcpy(&buf[20], &sz, sizeof(int));
	int offset = 24;
	for (int i(0); i < resultBuffer.size(); i++)
	{
		ContentConference * q = static_cast<ContentConference*>(resultBuffer[i]);
		addInt(buf, q->id, offset);
		addString(buf, q->name, offset);
		addString(buf, q->date, offset);
		addInt(buf, q->themeID, offset);
		addInt(buf, q->cityID, offset);
		addString(buf, q->description, offset);
		addInt(buf, q->adminID, offset);
		addString(buf, q->cityName, offset);
		addString(buf, q->themeName, offset);

		delete q;
	}

	resultBuffer.clear();

	printf("debug: ");
	for (int i(0); i < buf.size(); i++) printf("%c", buf[i]);
	printf("\n");

	SendToClient(&buf[0], buf.size(), ClientSocket, PacketID);
	resultBuffer.clear();
}

void HandleConferenceMembers(DataFormat data, SOCKET ClientSocket, std::vector<char> buf, int &PacketID, AccEnviroment &Env)
{
	std::string sqlText = "";
	sqlText += "SELECT Scientist.Surname, Scientist.Name, Scientist.id ";
	sqlText += "FROM Scientist, Participation ";
	sqlText += "WHERE Scientist.id = Participation.idScientist AND ";
	sqlText += "Participation.idConference = \'";

	int confId;
	memcpy(&confId, &buf[20], sizeof(int));

	char t[20] = "";
	_itoa_s(confId, t, 10);

	for (int i(0); i < strlen(t); i++) sqlText += t[i];
	sqlText += "\'";

	printf("debug: ");
	printf("%s\n", sqlText.c_str());

	ExecSQL(sqlText, DBCallbackFunc_Scientist);

	buf.clear();
	buf.assign(24, 0);
	data.ActionID = action_get_conf_members_response;
	memcpy(&buf[0], &data.Account, sizeof(__int64));
	memcpy(&buf[sizeof(__int64)], &data.ActionID, sizeof(Actions));
	int sz = resultBuffer.size();
	memcpy(&buf[20], &sz, sizeof(int));
	int offset = 24;
	for (int i(0); i < resultBuffer.size(); i++)
	{
		ContentScientist * q = static_cast<ContentScientist*>(resultBuffer[i]);
		addInt(buf, q->id, offset);
		addString(buf, q->surname, offset);
		addString(buf, q->name, offset);

		delete q;
	}

	resultBuffer.clear();

	printf("debug: ");
	for (int i(0); i < buf.size(); i++) printf("%c", buf[i]);
	printf("\n");

	SendToClient(&buf[0], buf.size(), ClientSocket, PacketID);
	resultBuffer.clear();

}

void HandleUpdateConfInfo(DataFormat data, SOCKET ClientSocket, std::vector<char> buf, int &PacketID, AccEnviroment &Env)
{
	
	if (Env.accType != 2)
	{
		data.ActionID = action_error;
		memcpy(&buf[sizeof(__int64)], &data.ActionID, sizeof(Actions));
	}

	int offset = 20;
	int size;

	readInt(buf, size, offset);

	ContentConference * q = new ContentConference();
	readInt(buf, q->id, offset);
	readString(buf, q->name, offset);
	readString(buf, q->date, offset);
	readInt(buf, q->themeID, offset);
	readInt(buf, q->cityID, offset);
	readString(buf, q->description, offset);
	readInt(buf, q->adminID, offset);
	readString(buf, q->cityName, offset);
	readString(buf, q->themeName, offset);

	std::string sqlText = "";

	/*GET CITY*/

	sqlText += "SELECT City.id FROM City WHERE City.Name = \'";
	for (char w : q->cityName) sqlText += w;
	sqlText += "\'";

	printf("debug: ");
	printf("%s\n", sqlText.c_str());

	ExecSQL(sqlText, DBCallbackFunc_Scientist);

	if (resultBuffer.empty())
	{
		buf.clear();
		buf.assign(24, 0);
		data.ActionID = action_error;
		memcpy(&buf[0], &data.Account, sizeof(__int64));
		memcpy(&buf[sizeof(__int64)], &data.ActionID, sizeof(Actions));
		SendToClient(&buf[0], buf.size(), ClientSocket, PacketID);
		return;
	}

	ContentScientist * tmp = static_cast<ContentScientist*>(resultBuffer[0]);
	q->cityID = tmp->id;
	delete(tmp);
	resultBuffer.clear();
	sqlText = "";
	
	/*GET THEME*/

	sqlText += "SELECT Theme.id FROM Theme WHERE Theme.Name = \'";
	for (char w : q->themeName) sqlText += w;
	sqlText += "\'";

	printf("debug: ");
	printf("%s\n", sqlText.c_str());
	ExecSQL(sqlText, DBCallbackFunc_Scientist);

	if (resultBuffer.empty())
	{
		buf.clear();
		buf.assign(24, 0);
		data.ActionID = action_error;
		memcpy(&buf[0], &data.Account, sizeof(__int64));
		memcpy(&buf[sizeof(__int64)], &data.ActionID, sizeof(Actions));
		SendToClient(&buf[0], buf.size(), ClientSocket, PacketID);
		return;
	}

	tmp = static_cast<ContentScientist*>(resultBuffer[0]);
	q->themeID = tmp->id;
	delete(tmp);
	resultBuffer.clear();
	sqlText = "";

	/*UPDATE*/

	sqlText += "Update Conference ";
	sqlText += "Set ";

	char t[20] = "";
	//_itoa_s(confId, t, 10);

	sqlText += "Name = \'";  
	for (char w : q->name) sqlText += w;
	sqlText += "\', ";

	sqlText += "Description = \'";
	for (char w : q->description) sqlText += w;
	sqlText += "\', ";

	sqlText += "Date = \'";
	for (char w : q->date) sqlText += w;
	sqlText += "\', ";
	
	for (int i(0); i < 20; i++) t[i] = 0;
	_itoa_s(q->themeID, t, 10);
	sqlText += "Theme = \'";
	for (int i(0); i<strlen(t); i++) sqlText += t[i];
	sqlText += "\', ";

	for (int i(0); i < 20; i++) t[i] = 0;
	_itoa_s(q->cityID, t, 10);
	sqlText += "City = \'";
	for (int i(0); i<strlen(t); i++) sqlText += t[i];
	sqlText += "\', ";

	for (int i(0); i < 20; i++) t[i] = 0;
	_itoa_s(q->adminID, t, 10);
	sqlText += "Admin = \'";
	for (int i(0); i<strlen(t); i++) sqlText += t[i];
	sqlText += "\' ";

	for (int i(0); i < 20; i++) t[i] = 0;
	_itoa_s(q->id, t, 10);
	sqlText += "WHERE Conference.id =  \'";
	for (int i(0); i<strlen(t); i++) sqlText += t[i];
	sqlText += "\'";

	printf("debug: ");
	printf("%s\n", sqlText.c_str());
	ExecSQL(sqlText, DBCallbackFunc_Test);

	buf.clear();
	buf.assign(24, 0);
	data.ActionID = action_update_conf_info_response;
	memcpy(&buf[0], &data.Account, sizeof(__int64));
	memcpy(&buf[sizeof(__int64)], &data.ActionID, sizeof(Actions));
	SendToClient(&buf[0], buf.size(), ClientSocket, PacketID);

	delete q;

}

void HandleLeaveConf(DataFormat data, SOCKET ClientSocket, std::vector<char> buf, int &PacketID, AccEnviroment &Env)
{

	int confId;
	memcpy(&confId, &buf[20], sizeof(int));

	char t[20] = "";
	_itoa_s(confId, t, 10);

	char r[20] = "";
	_itoa_s(Env.id, r, 10);

	std::string sqlText = "";
	sqlText += "DELETE FROM Participation ";
	sqlText += "WHERE Participation.idScientist in (SELECT Scientist.id FROM Scientist WHERE Scientist.Account = \'";
	for (int i(0); i < strlen(r); i++) sqlText += r[i];
	sqlText += "\') AND ";
	sqlText += "Participation.idConference = \'";
	for (int i(0); i < strlen(t); i++) sqlText += t[i];
	sqlText += "\'";

	printf("debug: ");
	printf("%s\n", sqlText.c_str());

	ExecSQL(sqlText, DBCallbackFunc_Scientist);

	buf.clear();
	buf.assign(24, 0);
	data.ActionID = action_leave_conf_response;
	memcpy(&buf[0], &data.Account, sizeof(__int64));
	memcpy(&buf[sizeof(__int64)], &data.ActionID, sizeof(Actions));

	SendToClient(&buf[0], buf.size(), ClientSocket, PacketID);
}

void HandleMessageList(DataFormat data, SOCKET ClientSocket, std::vector<char> buf, int &PacketID, AccEnviroment &Env)
{

	char r[20] = "";
	_itoa_s(Env.id, r, 10);

	std::string sqlText = "";
	sqlText += "SELECT Message.Caption, Message.Dest, Message.MsgType, Message.Scr, Message.Dest, Message.Status, Message.id , A1.Login as FName, A2.Login as TName ";
	sqlText += "FROM Message, Account as A1, Account as A2 ";
	sqlText += "WHERE (Dest = \'";

	for (int i(0); i < strlen(r); i++) sqlText += r[i];
	sqlText += "\' OR Scr = \'";
	for (int i(0); i < strlen(r); i++) sqlText += r[i];
	sqlText += "\')AND(A1.id = Message.Scr)AND(A2.id = Message.Dest)";
		
	printf("debug: ");
	printf("%s\n", sqlText.c_str());
		
	ExecSQL(sqlText, DBCallbackFunc_Message);

	buf.clear();
	buf.assign(24, 0);
	data.ActionID = action_message_list_response;
	memcpy(&buf[0], &data.Account, sizeof(__int64));
	memcpy(&buf[sizeof(__int64)], &data.ActionID, sizeof(Actions));

	int sz = resultBuffer.size();
	int offset = 20;
	addInt(buf, sz, offset);

	for (int i(0); i < resultBuffer.size(); i++)
	{
		ContentMessage * q = static_cast<ContentMessage*>(resultBuffer[i]);

		addInt(buf, q->id, offset);
		addInt(buf, q->scr, offset);
		addInt(buf, q->dest, offset);
		addString(buf, q->caption, offset);
		addInt(buf, q->MsgType, offset);
		q->caption = "NULL";
		addString(buf, q->scrName, offset);
		addString(buf, q->destName, offset);
	//	addInt(buf, q->MsgType, offset);
		addInt(buf, q->status, offset);
		delete q;
	}


	SendToClient(&buf[0], buf.size(), ClientSocket, PacketID);
/*
	for (int i(0); i < resultBuffer.size(); i++)
	{
		delete resultBuffer[i];
	}
	*/
	resultBuffer.clear();
}

void HandleGetMessage(DataFormat data, SOCKET ClientSocket, std::vector<char> buf, int &PacketID, AccEnviroment &Env)
{

	int msgId;
	memcpy(&msgId, &buf[20], sizeof(int));

	char t[20] = "";
	_itoa_s(msgId, t, 10);

	std::string sqlText = "";
	sqlText += "SELECT Message.Caption, Message.Dest, Message.MsgType, Message.Scr, Message.id, Message.Content, Message.Param1, Message.Status, A1.Login as FName, A2.Login as TName ";
	sqlText += "FROM Message, Account as A1, Account as A2 ";
	sqlText += "WHERE Message.id = \'";
	for (int i(0); i < strlen(t); i++) sqlText += t[i];
	sqlText += "\'AND(A1.id = Message.Scr)AND(A2.id = Message.Dest)";

	printf("debug: ");
	printf("%s\n", sqlText.c_str());

	ExecSQL(sqlText, DBCallbackFunc_Message);

	buf.clear();
	buf.assign(24, 0);
	data.ActionID = action_get_message_response;
	memcpy(&buf[0], &data.Account, sizeof(__int64));
	memcpy(&buf[sizeof(__int64)], &data.ActionID, sizeof(Actions));

	int sz = resultBuffer.size();
	int offset = 20;
	addInt(buf, sz, offset);
	int scrID = 0;
	for (int i(0); i < resultBuffer.size(); i++)
	{
		ContentMessage * q = static_cast<ContentMessage*>(resultBuffer[i]);

		addInt(buf, q->id, offset);
		addInt(buf, q->scr, offset);
		scrID = q->scr;
		addInt(buf, q->dest, offset);
		addString(buf, q->caption, offset);
		addInt(buf, q->MsgType, offset);
		q->caption = "NULL";
		addString(buf, q->scrName, offset);
		addString(buf, q->destName, offset);
		addString(buf, q->text, offset);
		addInt(buf, q->param1, offset);
		addInt(buf, q->status, offset);
		delete q;
	}


	SendToClient(&buf[0], buf.size(), ClientSocket, PacketID);

	if (scrID != Env.id)
	{
		sqlText = "";
		sqlText += "UPDATE Message ";
		sqlText += "SET Status = 2 ";
		sqlText += "WHERE id = \'";
		for (int i(0); i < strlen(t); i++) sqlText += t[i];
		sqlText += "\'";

		printf("debug: ");
		printf("%s\n", sqlText.c_str());
	}
	ExecSQL(sqlText, DBCallbackFunc_Message);

}

void HandleSearcConf(DataFormat data, SOCKET ClientSocket, std::vector<char> buf, int &PacketID, AccEnviroment &Env)
{
	std::string pattern;
	std::string city;
	std::string theme;
	int pflag;
	int cflag;
	int tflag;

	int idCity;
	int idTheme;

	int offset = 20;
	readString(buf, pattern, offset);
	readString(buf, city, offset);
	readString(buf, theme, offset);
	readInt(buf, pflag, offset);
	readInt(buf, cflag, offset);
	readInt(buf, tflag, offset);

	if (cflag == 1)
	{
		std::string sqlText = "";
		sqlText += "SELECT id FROM City WHERE Name = \'";
		for (char w : city) sqlText += w;
		sqlText += "\'";

		printf("debug: ");
		printf("%s\n", sqlText.c_str());

		ExecSQL(sqlText, DBCallbackFunc_Scientist);

		if (resultBuffer.empty())
		{
			int sz = 0;
			buf.clear();
			buf.assign(24, 0);
			data.ActionID = action_search_conf_response;
			memcpy(&buf[0], &data.Account, sizeof(__int64));
			memcpy(&buf[sizeof(__int64)], &data.ActionID, sizeof(Actions));
		//	int sz = resultBuffer.size();
			memcpy(&buf[20], &sz, sizeof(int));
			SendToClient(&buf[0], buf.size(), ClientSocket, PacketID);
			return;
		}

		ContentScientist * q = static_cast<ContentScientist*>(resultBuffer[0]);
		idCity = q->id;
		delete q;

	}


	if (tflag == 1)
	{
		std::string sqlText = "";
		sqlText += "SELECT id FROM Theme WHERE Name = \'";
		for (char w : theme) sqlText += w;
		sqlText += "\'";

		printf("debug: ");
		printf("%s\n", sqlText.c_str());

		ExecSQL(sqlText, DBCallbackFunc_Scientist);

		if (resultBuffer.empty())
		{
			int sz = 0;
			buf.clear();
			buf.assign(24, 0);
			data.ActionID = action_search_conf_response;
			memcpy(&buf[0], &data.Account, sizeof(__int64));
			memcpy(&buf[sizeof(__int64)], &data.ActionID, sizeof(Actions));
		//	int sz = resultBuffer.size();
			memcpy(&buf[20], &sz, sizeof(int));
			SendToClient(&buf[0], buf.size(), ClientSocket, PacketID);
			return;
		}

		ContentScientist * q = static_cast<ContentScientist*>(resultBuffer[0]);
		idTheme = q->id;
		delete q;

	}

	std::string sqlText = "";

	sqlText += "SELECT Conference.id, Conference.Name, ";
	sqlText += "Conference.Date, Conference.Theme, ";
	sqlText += "Conference.City, Conference.Description, ";
	sqlText += "Conference.Admin, City.Name as CName, ";
	sqlText += "Theme.Name as TName ";
	sqlText += "FROM Conference, City, Theme ";
	sqlText += "WHERE Conference.Theme = Theme.id AND ";
	sqlText += "Conference.City = City.id AND ";
	sqlText += "Conference.Name like \'%";

	for (char r : pattern) sqlText += r;

	sqlText +="%\'";

	if (cflag == 1)
	{
		sqlText += " AND City = \'";
		std::string s = std::to_string(idCity);
		for (char w : s) sqlText += w;
		sqlText += "\'";
	}

	if (tflag == 1)
	{
		sqlText += " AND Theme = \'";
		std::string s = std::to_string(idTheme);
		for (char w : s) sqlText += w;
		sqlText += "\'";
	}


	printf("debug: ");
	printf("%s\n", sqlText.c_str());

	ExecSQL(sqlText, DBCallbackFunc_ConferenceFull);

	buf.clear();
	buf.assign(24, 0);
	data.ActionID = action_search_conf_response;
	memcpy(&buf[0], &data.Account, sizeof(__int64));
	memcpy(&buf[sizeof(__int64)], &data.ActionID, sizeof(Actions));
	int sz = resultBuffer.size();
	memcpy(&buf[20], &sz, sizeof(int));
	offset = 24;
	for (int i(0); i < resultBuffer.size(); i++)
	{
		ContentConference * q = static_cast<ContentConference*>(resultBuffer[i]);
		addInt(buf, q->id, offset);
		addString(buf, q->name, offset);
		addString(buf, q->date, offset);
		addInt(buf, q->themeID, offset);
		addInt(buf, q->cityID, offset);
		addString(buf, q->description, offset);
		addInt(buf, q->adminID, offset);
		addString(buf, q->cityName, offset);
		addString(buf, q->themeName, offset);
		q->id = 1;
		addInt(buf, q->id, offset);
		// add int
		delete q;
	}

	resultBuffer.clear();

	printf("debug: ");
	for (int i(0); i < buf.size(); i++) printf("%c", buf[i]);
	printf("\n");

	SendToClient(&buf[0], buf.size(), ClientSocket, PacketID);
	resultBuffer.clear();

}

void HandleJoinConf(DataFormat data, SOCKET ClientSocket, std::vector<char> buf, int &PacketID, AccEnviroment &Env)
{
	char r[20] = "";
	_itoa_s(Env.id, r, 10);

	int confId;
	memcpy(&confId, &buf[20], sizeof(int));

	char t[20] = "";
	_itoa_s(confId, t, 10);

	std::string sqlText = "";

	sqlText += "SELECT * FROM Participation ";
	sqlText += "WHERE Participation.idScientist in (SELECT id FROM Scientist WHERE Scientist.Account = \'";
	for (int i(0); i < strlen(r); i++) sqlText += r[i];
	sqlText += "\') AND ";
	sqlText += "Participation.idConference = \'";
	for (int i(0); i < strlen(t); i++) sqlText += t[i];
	sqlText += "\'";

	printf("debug: ");
	printf("%s\n", sqlText.c_str());

	ExecSQL(sqlText, DBCallbackFunc_Partitipation);

	if (!resultBuffer.empty())
	{
		delete resultBuffer[0];
		buf.clear();
		buf.assign(24, 0);
		data.ActionID = action_error;
		memcpy(&buf[0], &data.Account, sizeof(__int64));
		memcpy(&buf[sizeof(__int64)], &data.ActionID, sizeof(Actions));

		SendToClient(&buf[0], buf.size(), ClientSocket, PacketID);
		return;
	}
	int sciID;
	sqlText = "";
	sqlText += "SELECT id FROM Scientist WHERE Account = \'";
	for (int i(0); i < strlen(r); i++) sqlText += r[i];

	sqlText += "\'";
	
	resultBuffer.clear();

	printf("debug: ");
	printf("%s\n", sqlText.c_str());

	ExecSQL(sqlText, DBCallbackFunc_Scientist);

	if (resultBuffer.empty())
	{
		delete resultBuffer[0];
		buf.clear();
		buf.assign(24, 0);
		data.ActionID = action_error;
		memcpy(&buf[0], &data.Account, sizeof(__int64));
		memcpy(&buf[sizeof(__int64)], &data.ActionID, sizeof(Actions));

		SendToClient(&buf[0], buf.size(), ClientSocket, PacketID);
		return;
	}
	else
	{
		ContentScientist * q = static_cast<ContentScientist*> (resultBuffer[0]);
		sciID = q->id;
		delete q;
	}

	resultBuffer.clear();

	char s[20] = "";
	_itoa_s(sciID, s, 10);

	sqlText = "";
	sqlText += "INSERT INTO Participation VALUES(";
	for (int i(0); i < strlen(s); i++) sqlText += s[i];
	sqlText += ",";
	for (int i(0); i < strlen(t); i++) sqlText += t[i];
	sqlText += ")";

	printf("debug: ");
	printf("%s\n", sqlText.c_str());

	ExecSQL(sqlText, DBCallbackFunc_Scientist);

	buf.clear();
	buf.assign(24, 0);
	data.ActionID = action_join_conf_response;
	memcpy(&buf[0], &data.Account, sizeof(__int64));
	memcpy(&buf[sizeof(__int64)], &data.ActionID, sizeof(Actions));

	SendToClient(&buf[0], buf.size(), ClientSocket, PacketID);
}

void HandleSendMessage(DataFormat data, SOCKET ClientSocket, std::vector<char> buf, int &PacketID, AccEnviroment &Env)
{

	int offset = 20;
	int size;

	int id;
	int scr;
	int dest;
	std::string caption;
	std::string text;
	int msgType;
	int param1;
	std::string fromName;
	std::string toName;
	int status = 1;

	readInt(buf, size, offset);
	readInt(buf, scr, offset);
	readInt(buf, dest, offset);
	readString(buf, caption, offset);
	readInt(buf, msgType, offset);
	readString(buf, fromName, offset);
	readString(buf, toName, offset);

	readString(buf, text, offset);
	readInt(buf, param1, offset);

	id = 0;
	scr = Env.id;



	std::string sql = "";
	sql += "SELECT id FROM Account WHERE Login = \'";
	for (char w : toName) sql += w;
	sql += "\'";

	printf("debug: ");
	printf("%s\n", sql.c_str());

	ExecSQL(sql, DBCallbackFunc_Account);

	if (resultBuffer.empty())
	{
		buf.clear();
		buf.assign(24, 0);
		data.ActionID = action_error;
		memcpy(&buf[0], &data.Account, sizeof(__int64));
		memcpy(&buf[sizeof(__int64)], &data.ActionID, sizeof(Actions));

		SendToClient(&buf[0], buf.size(), ClientSocket, PacketID);
		return;
	}

	ContentAccount * q = static_cast<ContentAccount*> (resultBuffer[0]);
	dest = q->id;
	delete q;

	sql = "";

	sql += "INSERT INTO Message(Scr, Dest, Caption, Content, MsgType, Param1, Status) ";
	sql += "VALUES(";
		//(1, 2, 'fsdfsf', 'fsdfsdfsdf', 1, NULL)
	std::string tmp = std::to_string(scr);

	for (char w : tmp) sql += w;
	sql += ",";

	tmp = std::to_string(dest);
	for (char w : tmp) sql += w;
	sql += ",\'";

	for (char w : caption) sql += w;
	sql += "\',\'";

	for (char w : text) sql += w;
	sql += "\',";

	tmp = std::to_string(msgType);
	for (char w : tmp) sql += w;
	sql += ',';

	tmp = std::to_string(param1);
	for (char w : tmp) sql += w;
	sql += ',';

	tmp = std::to_string(status);
	for (char w : tmp) sql += w;
	sql += ')';

	printf("debug: ");
	printf("%s\n", sql.c_str());

	ExecSQL(sql, DBCallbackFunc_Account);

	buf.clear();
	buf.assign(24, 0);
	data.ActionID = action_send_message_response;
	memcpy(&buf[0], &data.Account, sizeof(__int64));
	memcpy(&buf[sizeof(__int64)], &data.ActionID, sizeof(Actions));

	SendToClient(&buf[0], buf.size(), ClientSocket, PacketID);
}

void HandleAddConference(DataFormat data, SOCKET ClientSocket, std::vector<char> buf, int &PacketID, AccEnviroment &Env)
{





	std::string sqlText = "";
	sqlText += "SELECT Admin.id FROM Admin WHERE Admin.Account = \'";
	char r[20] = "";
	_itoa_s(Env.id, r, 10);
	for (int i(0); i < strlen(r); i++) sqlText += r[i];
	sqlText += "\'";

	printf("debug: ");
	printf("%s\n", sqlText.c_str());

	ExecSQL(sqlText, DBCallbackFunc_Scientist);

	ContentScientist * q = static_cast<ContentScientist*> (resultBuffer[0]);

	std::string admID = std::to_string(q->id);

	delete q;

	sqlText = "";
	sqlText += "INSERT INTO Conference(Name, City, Theme, Admin, Description, Date) ";
	sqlText += "VALUES('New conferece', 1, 1, ";
	for (char w : admID) sqlText += w;


	sqlText += ", 'Description', '01.01.2000') ";

	printf("debug: ");
	printf("%s\n", sqlText.c_str());

	ExecSQL(sqlText, DBCallbackFunc_Scientist);

	buf.clear();
	buf.assign(24, 0);
	data.ActionID = action_add_conference_responce;
	memcpy(&buf[0], &data.Account, sizeof(__int64));
	memcpy(&buf[sizeof(__int64)], &data.ActionID, sizeof(Actions));

	SendToClient(&buf[0], buf.size(), ClientSocket, PacketID);
}

void HandleDeleteConference(DataFormat data, SOCKET ClientSocket, std::vector<char> buf, int &PacketID, AccEnviroment &Env)
{
	int confId;
	std::string s;
	int offset = 20;
	readInt(buf, confId, offset);
	s = std::to_string(confId);

	std::string sqlText = "";

	sqlText += "DELETE FROM ";
	sqlText += "Participation ";
	sqlText += "WHERE ";
	sqlText += "Participation.idConference = \'";
	for (char w : s) sqlText += w;
	sqlText += "\'";

	printf("debug: ");
	printf("%s\n", sqlText.c_str());

	ExecSQL(sqlText, DBCallbackFunc_Scientist);


	sqlText = "";
	sqlText += "DELETE FROM ";
	sqlText += "CONFERENCE ";
	sqlText += "WHERE ";
	sqlText += "id = \'";
	for (char w : s) sqlText += w;


	sqlText += "\'";


	printf("debug: ");
	printf("%s\n", sqlText.c_str());
	ExecSQL(sqlText, DBCallbackFunc_Scientist);

	buf.clear();
	buf.assign(24, 0);
	data.ActionID = action_delete_conf_response;
	memcpy(&buf[0], &data.Account, sizeof(__int64));
	memcpy(&buf[sizeof(__int64)], &data.ActionID, sizeof(Actions));

	SendToClient(&buf[0], buf.size(), ClientSocket, PacketID);
}

void HandleGetProfile(DataFormat data, SOCKET ClientSocket, std::vector<char> buf, int &PacketID, AccEnviroment &Env)
{

	std::string sqlText = "";

	sqlText += "SELECT * FROM Account ";
	sqlText += "WHERE ";
	sqlText += "id = \'";

	int id = Env.id;
	std::string s;
	int offset = 20;
	//readInt(buf, id, offset);
	s = std::to_string(id);

	for (char w : s) sqlText += w;

	sqlText += "\'";


	printf("debug: ");
	printf("%s\n", sqlText.c_str());
	ExecSQL(sqlText, DBCallbackFunc_Account);

	ContentAccount * q = static_cast<ContentAccount*>(resultBuffer[0]);

	buf.clear();
	buf.assign(24, 0);
	data.ActionID = action_get_profile_response;
	memcpy(&buf[0], &data.Account, sizeof(__int64));
	memcpy(&buf[sizeof(__int64)], &data.ActionID, sizeof(Actions));

	addString(buf, q->login, offset);

	delete q;


	if (Env.accType == 1)
	{

		sqlText = "";

		sqlText += "SELECT Scientist.Name, Scientist.Surname, City.Name as CName, ";
		sqlText += "Theme.Name as TName ";
		sqlText += "FROM ";
		sqlText += "Scientist, Theme, City ";
		sqlText += "Where Theme.id = Scientist.Theme AND ";
		sqlText += "City.id = Scientist.City AND ";
		sqlText += "Scientist.Account = \'";

		for (char w : s) sqlText += w;

		sqlText += "\'";

		printf("debug: ");
		printf("%s\n", sqlText.c_str());
		ExecSQL(sqlText, DBCallbackFunc_Scientist);

		ContentScientist * q = static_cast<ContentScientist*> (resultBuffer[0]);

		addString(buf, q->name, offset);
		addString(buf, q->surname, offset);
		addString(buf, q->themeName, offset);
		addString(buf, q->cityName, offset);

	}

	SendToClient(&buf[0], buf.size(), ClientSocket, PacketID);

}

void HandleCreateAccount(DataFormat data, SOCKET ClientSocket, std::vector<char> buf, int &PacketID, AccEnviroment &Env)
{

	std::string login;
	std::string password;
	std::string name;
	std::string surname;
	std::string city;
	std::string theme;

	int idCity;
	int idTheme;

	int offset = 20;

	readString(buf, login, offset);
	readString(buf, password, offset);
	readString(buf, name, offset);
	readString(buf, surname, offset);
	readString(buf, city, offset);
	readString(buf, theme, offset);

	{
		std::string sqlText = "";
		sqlText += "SELECT id FROM City WHERE Name = \'";
		for (char w : city) sqlText += w;
		sqlText += "\'";

		printf("debug: ");
		printf("%s\n", sqlText.c_str());
		ExecSQL(sqlText, DBCallbackFunc_Scientist);

		if (resultBuffer.empty())
		{
			buf.clear();
			buf.assign(24, 0);
			data.ActionID = action_error;
			memcpy(&buf[0], &data.Account, sizeof(__int64));
			memcpy(&buf[sizeof(__int64)], &data.ActionID, sizeof(Actions));
			SendToClient(&buf[0], buf.size(), ClientSocket, PacketID);
			return;
		}

		ContentScientist * q = static_cast<ContentScientist*>(resultBuffer[0]);

		idCity = q->id;

		delete q;
	}
	{
		std::string sqlText = "";
		sqlText += "SELECT id FROM Theme WHERE Name = \'";
		for (char w : theme) sqlText += w;
		sqlText += "\'";

		printf("debug: ");
		printf("%s\n", sqlText.c_str());
		ExecSQL(sqlText, DBCallbackFunc_Scientist);

		if (resultBuffer.empty())
		{
			buf.clear();
			buf.assign(24, 0);
			data.ActionID = action_error;
			memcpy(&buf[0], &data.Account, sizeof(__int64));
			memcpy(&buf[sizeof(__int64)], &data.ActionID, sizeof(Actions));
			SendToClient(&buf[0], buf.size(), ClientSocket, PacketID);
			return;
		}

		ContentScientist * q = static_cast<ContentScientist*>(resultBuffer[0]);

		idTheme = q->id;

		delete q;
	}

	{
		std::string sqlText = "";
		sqlText += "SELECT id FROM Account WHERE Login = \'";
		for (char w : login) sqlText += w;
		sqlText += "\'";

		printf("debug: ");
		printf("%s\n", sqlText.c_str());
		ExecSQL(sqlText, DBCallbackFunc_Account);

		if (!resultBuffer.empty())
		{
			buf.clear();
			buf.assign(24, 0);
			data.ActionID = action_error;
			memcpy(&buf[0], &data.Account, sizeof(__int64));
			memcpy(&buf[sizeof(__int64)], &data.ActionID, sizeof(Actions));
			SendToClient(&buf[0], buf.size(), ClientSocket, PacketID);
			return;
		}
		/*
		ContentScientist * q = static_cast<ContentScientist*>(resultBuffer[0]);

		idTheme = q->id;

		delete q;*/
	}

	std::string sqlText = "";
	sqlText += "INSERT INTO Account(Login, Password, AccountType) ";
	sqlText += "VALUES(\'";
	for (char w : login) sqlText += w;
	sqlText += "\',\'";
	for (char w : password) sqlText += w;
	sqlText += "\',\'1\')";


	printf("debug: ");
	printf("%s\n", sqlText.c_str());
	ExecSQL(sqlText, DBCallbackFunc_Scientist);

	sqlText = "";

	sqlText += "SELECT id FROM Account WHERE Login = \'";
	for (char w : login) sqlText += w;
	sqlText += "\'";

	printf("debug: ");
	printf("%s\n", sqlText.c_str());
	ExecSQL(sqlText, DBCallbackFunc_Account);

	ContentAccount * q = static_cast<ContentAccount*>(resultBuffer[0]);
	int idAcc;
	idAcc = q->id;
	std::string account;

	account = std::to_string(idAcc);
	city = std::to_string(idCity);
	theme = std::to_string(idTheme);

	delete q;

	sqlText = "";
	sqlText += "INSERT INTO Scientist(Surname, Name, City, Theme, Account) ";
	sqlText += "VALUES (\'";
	for (char w : surname) sqlText += w;
	sqlText += "\',\'";
	for (char w : name) sqlText += w;
	sqlText += "\',\'";
	for (char w : city) sqlText += w;
	sqlText += "\',\'";
	for (char w : theme) sqlText += w;
	sqlText += "\',\'";
	for (char w : account) sqlText += w;
	sqlText += "\')";

	printf("debug: ");
	printf("%s\n", sqlText.c_str());
	ExecSQL(sqlText, DBCallbackFunc_Scientist);

	buf.clear();
	buf.assign(24, 0);
	data.ActionID = action_create_account_response;
	memcpy(&buf[0], &data.Account, sizeof(__int64));
	memcpy(&buf[sizeof(__int64)], &data.ActionID, sizeof(Actions));
	SendToClient(&buf[0], buf.size(), ClientSocket, PacketID);
}

void HandleAddTheme(DataFormat data, SOCKET ClientSocket, std::vector<char> buf, int &PacketID, AccEnviroment &Env)
{
	std::string str;
	int offset = 20;
	readString(buf, str, offset);

	std::string sqlText = "";
	sqlText += "SELECT id FROM Theme WHERE Name = \'";
	for (char w : str) sqlText += w;
	sqlText += "\'";

	printf("debug: ");
	printf("%s\n", sqlText.c_str());
	ExecSQL(sqlText, DBCallbackFunc_Scientist);

	if (!resultBuffer.empty())
	{
		buf.clear();
		buf.assign(24, 0);
		data.ActionID = action_error;
		memcpy(&buf[0], &data.Account, sizeof(__int64));
		memcpy(&buf[sizeof(__int64)], &data.ActionID, sizeof(Actions));
		SendToClient(&buf[0], buf.size(), ClientSocket, PacketID);
		return;
	}

	sqlText = "";
	sqlText += "INSERT INTO Theme (Name) VALUES (\'";
	for (char w : str) sqlText += w;
	sqlText += "\')";


	printf("debug: ");
	printf("%s\n", sqlText.c_str());
	ExecSQL(sqlText, DBCallbackFunc_Scientist);

	buf.clear();
	buf.assign(24, 0);
	data.ActionID = action_add_theme_response;
	memcpy(&buf[0], &data.Account, sizeof(__int64));
	memcpy(&buf[sizeof(__int64)], &data.ActionID, sizeof(Actions));
	SendToClient(&buf[0], buf.size(), ClientSocket, PacketID);

}

void HandleAddCity(DataFormat data, SOCKET ClientSocket, std::vector<char> buf, int &PacketID, AccEnviroment &Env)
{
	std::string str;
	int offset = 20;
	readString(buf, str, offset);

	std::string sqlText = "";
	sqlText += "SELECT id FROM City WHERE Name = \'";
	for (char w : str) sqlText += w;
	sqlText += "\'";

	printf("debug: ");
	printf("%s\n", sqlText.c_str());
	ExecSQL(sqlText, DBCallbackFunc_Scientist);

	if (!resultBuffer.empty())
	{
		buf.clear();
		buf.assign(24, 0);
		data.ActionID = action_error;
		memcpy(&buf[0], &data.Account, sizeof(__int64));
		memcpy(&buf[sizeof(__int64)], &data.ActionID, sizeof(Actions));
		SendToClient(&buf[0], buf.size(), ClientSocket, PacketID);
		return;
	}

	sqlText = "";
	sqlText += "INSERT INTO City (Name) VALUES (\'";
	for (char w : str) sqlText += w;
	sqlText += "\')";


	printf("debug: ");
	printf("%s\n", sqlText.c_str());
	ExecSQL(sqlText, DBCallbackFunc_Scientist);

	buf.clear();
	buf.assign(24, 0);
	data.ActionID = action_add_city_response;
	memcpy(&buf[0], &data.Account, sizeof(__int64));
	memcpy(&buf[sizeof(__int64)], &data.ActionID, sizeof(Actions));
	SendToClient(&buf[0], buf.size(), ClientSocket, PacketID);

}