#include "SysMsgHandler.h"
#include "Reflection.h"
#include "Singleton.h"
#include "ClientState.h"
#include "NetManager.h"

SysMsgHandler::SysMsgHandler()
{

}

SysMsgHandler::~SysMsgHandler()
{

}

void SysMsgHandler::RegisterFun()
{
	REGISTE_FUN(SysMsgHandler, MsgPing);
}

void SysMsgHandler::MsgPing(ClientState * client, BaseMsg * msg)
{
	//TODO �յ�ping ˢ��ʱ�䲢�ظ�Pong
	printf("Ping\n");
	//client->SetPingTime();

	//c.lastPingTime = NetManager.GetTimeStamp();
	//MsgPong msgPong = new MsgPong();
	//NetManager.Send(c, msgPong);
}
