#include "EventHandler.h"
#include "Logger.h"

EventHandler::EventHandler()
{

}

EventHandler::~EventHandler()
{

}

void EventHandler::OnDisconnect(ClientState * state)
{
	LogInfo("OnDisconnect");
	//Console.WriteLine("Close");
	////Player����
	//if (c.player != null) {
	//	//�뿪ս��
	//	int roomId = c.player.roomId;
	//	if (roomId >= 0) {
	//		Room room = RoomManager.GetRoom(roomId);
	//		room.RemovePlayer(c.player.id);
	//	}
	//	//��������
	//	DbManager.UpdatePlayerData(c.player.id, c.player.data);
	//	//�Ƴ�
	//	PlayerManager.RemovePlayer(c.player.id);
	//}
}
