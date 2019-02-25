#ifndef __ROOM_SESSION_H__
#define __ROOM_SESSION_H__

#include "Player.h"
#include "TcpSession.h"

#include <memory>

class TcpConnection;

struct OnlineUserInfo
{
	int32_t     userid;
	std::string username;
	std::string nickname;
	std::string password;
	int32_t     clienttype;     //�ͻ�������, 0δ֪, pc=1, android/ios=2
	int32_t     status;         //����״̬ 0���� 1���� 2æµ 3�뿪 4����
};

class RoomSession:public TcpSession
{
public:
	RoomSession();
	RoomSession(const std::shared_ptr<TcpConnection>& conn, int sessionid);
	~RoomSession();
	Player* AddPlayer(TcpConnection* conn);
	bool RsEnoughPlayer();

	int32_t GetUserId()
	{
		return m_userinfo_.userid;
	}
	void OnRead(TcpConnection* conn, Buffer * pBuffer);
private:
	Player player_[4];
	int player_count_;
	OnlineUserInfo    m_userinfo_;
	int m_id_;
};
#endif // !__ROOM_SESSION_H__
