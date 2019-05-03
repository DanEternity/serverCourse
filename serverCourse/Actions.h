#pragma once

enum Actions
{
	action_ping = 0,
	action_ping_response,



};

struct DataFormat
{
	__int64 Account;
	Actions ActionID;
	int PacketID;
	int PacketCountExpected;
};