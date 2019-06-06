#pragma once

#include "SQLite3.h"
#include <vector>
#include <string>

static sqlite3 * dataBase = NULL;

struct ContentBase;

std::vector<ContentBase*> resultBuffer;

// DB OBJECTS

struct ContentBase
{

};

struct ContentScientist : ContentBase
{
	int id;
	std::string name;
	std::string surname;
	int cityID;
	int themeID;

	std::string cityName;
	std::string themeName;

	int accountID;
};

struct ContentCity : ContentBase
{
	int id;
	std::string name;
	std::string postIndex;
};

struct ContentTheme : ContentBase
{
	int id;
	std::string name;
};

struct ContentAccount : ContentBase
{
	int id;
	std::string login;
	std::string password;
	int accType;
};

struct ContentConference : ContentBase
{
	int id;
	std::string name;
	std::string description;
	int cityID;
	int themeID;
	int adminID;
	std::string date;

	// Also parsed date
	int year;
	int month;
	int day;
	int hour; // ???
	int minute; // ???
	
};

struct ContentParticipation : ContentBase
{
	int ScientistID;
	int ConferenceID;
};

struct ContentAdmin : ContentBase
{
	int id;
	int accID;
};