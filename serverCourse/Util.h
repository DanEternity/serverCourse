#pragma once

#include <vector>
#include <algorithm>
#include <string>

#undef min;
struct tBuff
{
	char SIG[5] = "PAC1";
	__int32 size = 0; // сколько всего должно быть пакетов
	__int32 number = 0; // номер пакета
	__int32 id = 0; // индекс группы пакетов

	char data[200];

	__int32 stop = INT_MAX;
};

std::vector<std::vector<char>> RawToBuff(char * rawData, int size, int id)
{

	int groups = (size / 200) + ((size % 200 > 0) ? 1 : 0);

	tBuff sm = tBuff();
	char buff[512];
	
	std::vector<std::vector<char>> p;

	for (int i(0); i < groups; i++)
	{
		memcpy(&buff[0], sm.SIG, 4U);
		memcpy(&buff[4], &groups, sizeof(int));
		memcpy(&buff[8], &i, sizeof(int));
		memcpy(&buff[12], &id, sizeof(int));
		memcpy(&buff[16], &rawData[i * 200], sizeof(char) * std::min(200, size - i * 200));
		memcpy(&buff[std::min(200, size - i * 200) + 16], &sm.stop, sizeof(int));
		std::vector<char> w;
		for (int h(0); h < 512; h++) w.push_back(buff[h]);
		p.push_back(w);
	}

	return p;
}

std::vector<char> BuffToRaw(std::vector<std::vector<char>> &d)
{

	std::vector<char> p;
	p.resize(d.size() * 200);
	for (int i(0); i < d.size(); i++)
	{
		int id = 0;

		memcpy(&id, &d[i][8], sizeof(int));

		memcpy(&p[id * 200], &d[i][16], sizeof(char) * 200);

	}

	return p;
}