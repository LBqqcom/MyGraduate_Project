#ifndef __IM_SERVER_H__
#define __IM_SERVER_H__

#include "EventLoop.h"
#include "TcpConnection.h"
#include "TcpServer.h"
#include "RoomSession.h"

#include <memory>
#include <list>
#include <map>
#include <mutex>

//�������session
class IMServer
{
public:
	IMServer() = default;
	~IMServer() = default;

	IMServer(const IMServer& rhs) = delete;
	IMServer& operator =(const IMServer& rhs) = delete;

	bool Init( EventLoop* loop);

	void GetSessions(std::list<std::shared_ptr<RoomSession>>& sessions);
	//�û�id��clienttype��Ψһȷ��һ��session
	bool GetSessionByUserIdAndClientType(std::shared_ptr<RoomSession>& session, int32_t userid, int32_t clientType);

	bool GetSessionsByUserId(std::list<std::shared_ptr<RoomSession>>& sessions, int32_t userid);

	//��ȡ�û�״̬�������û������ڣ��򷵻�0
	int32_t GetUserStatusByUserId(int32_t userid);
	//��ȡ�û��ͻ������ͣ�������û������ڣ��򷵻�0
	int32_t GetUserClientTypeByUserId(int32_t userid);

private:
	//�����ӵ������û����ӶϿ���������Ҫͨ��conn->connected()���жϣ�һ��ֻ����loop�������
	void OnConnection(std::shared_ptr<TcpConnection> conn);
	//���ӶϿ�
	void OnClose(const std::shared_ptr<TcpConnection>& conn);
private:
	std::shared_ptr<TcpServer>                     m_server;
	std::list<std::shared_ptr<RoomSession>>        m_sessions;
	std::mutex                                     m_sessionMutex;      //���߳�֮�䱣��m_sessions
	int                                            m_sessionId{};
	std::mutex                                     m_idMutex;           //���߳�֮�䱣��m_baseUserId
};

#endif // !__IM_SERVER_H__
