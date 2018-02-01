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