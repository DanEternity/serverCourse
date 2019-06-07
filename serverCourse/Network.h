#pragma once

#include <WinSock2.h>
#include <vector>
#include "Util.h"

#define DEFAULT_BUFF_TIME 10000

int SendToClient(char * buffer, int size, SOCKET socket, int &PacketID)
{
	/**/
	std::vector<std::vector<char>> r = RawToBuff(buffer, size, ++PacketID);

	for (int i(0); i < r.size(); i++)
	{
		char * q = &r[i][0];
		send(socket, q, 512, 0);
	}
	return 0;
}

int AddToQueue(std::vector<char> rBuff, std::vector<std::pair<int, std::vector<std::vector<char>>>> &bf, std::vector<std::pair<int, int>> &status)
{
	int bufId;
	memcpy(&bufId, &rBuff[12], sizeof(int));
	bool found = false;
	for (int i(0); i < bf.size(); i++)
	{
		if (bufId == bf[i].first)
		{
			//std::vector<char> t;
			//t.resize(DEFAULT_BUFLEN);
			//memcpy(rBuff[0], &t[0], DEFAULT_BUFLEN);
			bf[i].second.push_back(rBuff);
			return 0;
		}
	}

	std::pair<int, std::vector<std::vector<char>>> elem;

	elem.first = bufId;
	//std::vector<char> t;
	//t.resize(DEFAULT_BUFLEN);
	//memcpy(recvbuf, &t[0], DEFAULT_BUFLEN);
	elem.second.push_back(rBuff);
	bf.push_back(elem);
	status.push_back(std::make_pair(0, 0));

	return 0;
}

int CheckBufferStatus(std::vector<std::pair<int, std::vector<std::vector<char>>>> &bf, std::vector<std::pair<int, int>> &status)
{

	for (int i(0); i < bf.size(); i++)
	{
		int size;
		memcpy(&size, &bf[i].second[0][4], sizeof(int));
		std::vector<bool> parts(size, false);
		for (int j(0); j < bf[i].second.size(); j++)
		{
			int num;
			memcpy(&num, &bf[i].second[j][8], sizeof(int));
			parts[num] = true;
		}

		bool ready = true;

		for (int j(0); j<parts.size(); j++)
			if (parts[j] == false)
			{
				status[i].first++;
				ready = false;
				break;
			}

		if (ready)
		{
			status[i].first = DEFAULT_BUFF_TIME + 1;
			return i;
		}
	}

	return -1;
}

int ClearBuffer(std::vector<std::pair<int, std::vector<std::vector<char>>>> &bf, std::vector<std::pair<int, int>> &status)
{
	for (int i(0); i < bf.size(); i++)
	{
		if (status[i].first > DEFAULT_BUFF_TIME)
		{
			bf.erase(bf.begin() + i);
			status.erase(status.begin() + i);
			i--;
		}
	}

	return 0;
}