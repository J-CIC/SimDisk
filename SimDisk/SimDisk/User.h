#pragma once
class User
{
public:
	char username[17];//�16λ������
	char password[33];//�32λ������
	int g_id;//�û����1Ϊ����Ա��2Ϊ��ͨ
	int u_id;//�û�id
	bool is_curr;//�Ƿ�ǰ�����û�
	User();
	User(string name, string pwd,int uid);
	~User();
};

