#pragma once

#include "SQLite3.h"
#include "dbValues.h"
#include <cstdio>
#include <string>

void initDB(const char * filename)
{

	if (sqlite3_open(filename, &dataBase) != SQLITE_OK) {
		printf("ERROR: can't open database: %s\n", sqlite3_errmsg(dataBase));
	}
	else { printf("Connection Successful\n"); }

}

void closeDB()
{

	sqlite3_close(dataBase);

}

static int callback(void *NotUsed, int argc, char **argv, char **azColName) 
{
	int i;
	for (i = 0; i<argc; i++) 
	{
		printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	}
	printf("\n");
	return 0;
}

static int callback2(void *NotUsed, int argc, char **argv, char **azColName)
{
	ContentScientist * q = new ContentScientist();
	int i;
	for (i = 0; i<argc; i++)
	{
	//	printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
		switch (azColName[i][0])
		{
		case 'S':
			q->surname = argv[i];
			break;
		case 'N':
			q->name = argv[i];
			break;
		case 'A':
			q->accountID = std::atoi(argv[i]);
			break;
		case 'T':
			q->themeID = std::atoi(argv[i]);
			break;
		case 'i':
			q->id = std::atoi(argv[i]);
			break;
		case 'C':
			q->cityID = std::atoi(argv[i]);
			break;
		default:
			break;
		}
	}
//	printf("\n");
	resultBuffer.push_back(static_cast<ContentBase*>(q));
	return 0;
}

static int callbackAccount(void *NotUsed, int argc, char **argv, char **azColName)
{
	ContentAccount * q = new ContentAccount();
	int i;
	for (i = 0; i < argc; i++)
	{
		switch (azColName[i][0])
		{
		case 'L':
			q->login = argv[i];
			break;
		case 'P':
			q->password = argv[i];
			break;
		case 'i':
			q->id = std::atoi(argv[i]);
			break;
		case 'A':
			q->accType = std::atoi(argv[i]);
			break;
		default:
			break;
		}

	}

	resultBuffer.push_back(static_cast<ContentBase*>(q));
	return 0;
}

static int callbackConference(void *NotUsed, int argc, char **argv, char **azColName)
{
	ContentConference * q = new ContentConference();
	int i;
	for (i = 0; i < argc; i++)
	{
		switch (azColName[i][0])
		{
		case 'N':
			q->name = argv[i];
			break;
		case 'i':
			q->id = std::atoi(argv[i]);
			break;
		default:
			break;
		}

	}

	resultBuffer.push_back(static_cast<ContentBase*>(q));
	return 0;
}

enum DBCallbackFunc
{
	DBCallbackFunc_Test = 0,
	DBCallbackFunc_Scientist,
	DBCallbackFunc_Account,
	DBCallbackFunc_Conference,
};

void ExecSQL(std::string sqlString, DBCallbackFunc func)
{
	
	int result;

	char *zErrMsg = 0;

	switch (func)
	{
	case DBCallbackFunc_Test:
		result = sqlite3_exec(dataBase, sqlString.c_str(), callback, 0, &zErrMsg);
		break;
	case DBCallbackFunc_Scientist:
		resultBuffer.clear();
		result = sqlite3_exec(dataBase, sqlString.c_str(), callback2, 0, &zErrMsg);
		break;
	case DBCallbackFunc_Account:
		resultBuffer.clear();
		result = sqlite3_exec(dataBase, sqlString.c_str(), callbackAccount, 0, &zErrMsg);
		break;
	case DBCallbackFunc_Conference:
		resultBuffer.clear();
		result = sqlite3_exec(dataBase, sqlString.c_str(), callbackConference, 0, &zErrMsg);
		break;
	default:
		result = -1; // fail
		break;
	}

    if (result != SQLITE_OK) 
	{
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}

}

void Test()
{
	std::string sqlText = "SELECT * FROM \"Scientist\"";
	
	ExecSQL(sqlText, DBCallbackFunc_Scientist);

}