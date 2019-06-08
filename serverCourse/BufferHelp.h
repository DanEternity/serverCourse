#pragma once

#include <vector>
#include <string>

void addInt(std::vector<char> &buf, int x, int &offset)
{
	buf.resize(buf.size() + 4);
	memcpy(&buf[0 + offset], &x, sizeof(x));
	offset += 4;
}

void addChars(std::vector<char> &buf, const char * s, int size, int &offset)
{
	buf.resize(buf.size() + size);
	addInt(buf, size, offset);
	memcpy(&buf[0 + offset], &s, sizeof(char) * size);
	offset += 4 + size;
}

void addString(std::vector<char> &buf, std::string s, int &offset)
{
	buf.resize(buf.size() + s.size());
	addInt(buf, s.size(), offset);
	if (!s.empty())
		memcpy(&buf[0 + offset], s.c_str(), sizeof(char) * s.size());
	offset += s.size();
}

void readInt(std::vector<char> &buf, int &x, int &offset)
{
	memcpy(&x, &buf[0 + offset], sizeof(x));
	offset += 4;
}

void readString(std::vector<char> &buf, std::string &s, int &offset)
{
	int size;
	readInt(buf, size, offset);
	if (size != 0)
	{
		s.resize(size, 0);
		memcpy(&s[0], &buf[0 + offset], sizeof(char) * s.size());
	}
	else
		s = "";
	offset += s.size();
}