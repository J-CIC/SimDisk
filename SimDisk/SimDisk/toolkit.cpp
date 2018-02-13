#include "stdafx.h"
#include "toolkit.h"
#include <algorithm>
string num2permission(int n)
{
	string ret = "----------";
	unsigned short mask = 1;
	unsigned short use_mask = mask << 14;
	if (n&use_mask){
		ret[0] = 'd';
	}
	char res[3] = { 'r', 'w', 'x' };
	for (int i = 0; i < 3; i++){
		for (int j = 0; j < 3; j++){
			use_mask = mask << (i + j * 4);
			int pos = 7 + i - j * 3;
			if (use_mask&n){
				ret[pos] = res[i];
			}
		}
	}
	return ret;
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