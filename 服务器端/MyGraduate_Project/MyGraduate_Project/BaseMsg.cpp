#include "BaseMsg.h"
#include "MsgCreateTurret.h"
#include "MsgTurretInfo.h"
#include "MsgPing.h"

#define RETURN_MSG(x) 	if (strcmp(protoName,#x)== 0)\
	{\
	return new x();\
	}

BaseMsg::BaseMsg()
{

}


BaseMsg::~BaseMsg()
{

}

void BaseMsg::PrintData()
{

}

void BaseMsg::Init()
{

}

char * BaseMsg::EnCode(int &dataLen)
{
	return nullptr;
}

BaseMsg * BaseMsg::DeCode(Json::Value)
{
	return this;
}

BaseMsg * BaseMsg::DeCode(char * datas)
{
	Json::Reader reader;
	Json::Value value;

	int prefixionLen = GET_INT16(datas + 2) + 4;	//����int_16����Э�����ĳ���

	if (reader.parse(datas + prefixionLen , value))
	{
		BaseMsg *newBaseMsg = GetMsg(value["_protoName"].asString().c_str());
		return newBaseMsg->DeCode(value);
	}
	return NULL;
}

BaseMsg * BaseMsg::GetMsg(const char * protoName)
{
	RETURN_MSG(MsgCreateTurret);
	RETURN_MSG(MsgTurretInfo);
	RETURN_MSG(MsgPing);
	return nullptr;
}