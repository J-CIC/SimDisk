#include "stdafx.h"
#include "User.h"


User::User()
{
}

User::User(string name, string pwd,int uid)
{
	strncpy_s(username, name.c_str(), 16);//复制用户名
	strncpy_s(password, name.c_str(), 32);//复制密码
	username[16] = '\0';
	password[32] = '\0';
	this->u_id = uid;//设置uid
	this->g_id = uid;
}

User::~User()
{
}

bool User::auth(char* usr_name, char* usr_pwd){

	if (strcmp(usr_name, username)==0&&strcmp(usr_pwd,password)==0){
		//认证成功
		return true;
	}
	return false;
}