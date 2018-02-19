#pragma once
class User
{
public:
	char username[17];//最长16位的密码
	char password[33];//最长32位的密码
	int g_id;//用户类别，1为管理员，2为普通
	int u_id;//用户id
	bool is_curr;//是否当前启用用户
	User();
	User(string name, string pwd,int uid);
	~User();
};

