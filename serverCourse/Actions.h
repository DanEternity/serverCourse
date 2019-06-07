#pragma once

enum Actions
{
	action_ping = 0,
	action_ping_response,
	action_auth,
	action_auth_fail,
	action_auth_success,
	action_conf_user,
	action_conf_user_responce,
};

struct DataFormat
{
	__int64 Account;
	Actions ActionID;
	int PacketID;
	int PacketCountExpected;
};