#include "stdafx.h"
#include "toolkit.h"
#include <algorithm>
string num2bin(int n)
{
	int a;
	int count = 0;
	string str;
	while (n != 0){
		a = n % 2;
		n = n >> 1;
		string temp = a ? "1" : "0";
		str.append(temp);
		if (++count % 4 == 0){
			str.append(" ");
		}
	}
	reverse(str.begin(), str.end());
	return str;
}

void SplitString(const std::string& s, std::vector<std::string>& v, const std::string& c)
{
	std::string::size_type pos1, pos2;
	pos2 = s.find(c);
	pos1 = 0;
	while (std::string::npos != pos2)
	{
		v.push_back(s.substr(pos1, pos2 - pos1));

		pos1 = pos2 + c.size();
		pos2 = s.find(c, pos1);
	}
	if (pos1 != s.length())
		v.push_back(s.substr(pos1));
}