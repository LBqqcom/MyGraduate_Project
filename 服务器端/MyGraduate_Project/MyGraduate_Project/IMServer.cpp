#include "IMServer.h"
#include "Logger.h"
///�ƹ����е�RoomSession

bool IMServer::Init(EventLoop * loop)
{
	//���÷���������ָ��ָ��
	m_server.reset(new TcpServer(loop));

	//���ý������ӻص�
	m_server->setConnectionCallback(std::bind(&IMServer::OnConnection, this, std::placeholders::_1));
	//��������
	m_server->Start();

	return true;
}

void IMServer::GetSessions(std::list<std::shared_ptr<RoomSession>>& sessions)
{
	std::lock_guard<std::mutex> guard(m_sessionMutex);
	sessions = m_sessions;
}

bool IMServer::GetSessionByUserIdAndClientType(std::shared_ptr<RoomSession>& session, int32_t userid, int32_t clientType)
{
	return false;
}

bool IMServer::GetSessionsByUserId(std::list<std::shared_ptr<RoomSession>>& sessions, int32_t userid)
{
	std::lock_guard<std::mutex> guard(m_sessionMutex);
	std::shared_ptr<RoomSession> tmpSession;
	for (const auto& iter : m_sessions)
	{
		tmpSession = iter;
		if (iter->GetUserId() == userid)
		{
			sessions.push_back(tmpSession);
			return true;
		}
	}

	return false;
}

int32_t IMServer::GetUserStatusByUserId(int32_t userid)
{
	return 0;
}

int32_t IMServer::GetUserClientTypeByUserId(int32_t userid)
{
	return int32_t();
}
//��connection������ɻ������ӶϿ�ʱ���ô˺������д���
void IMServer::OnConnection(std::shared_ptr<TcpConnection> conn)
{
	//���ӳɹ��򴴽�һ���Ự	���ҽ�conn��ҵ�����ݴ���ص�ָ��ΪRoomSession��OnRread����
	if (conn->connected())
	{
		//LOG_INFO << "client connected:" << conn->peerAddress().toIpPort();
		LogInfo("create a new connection session");
		//TODO �޸�Ϊ������Session���� ����һ������NetManager�� ���������ClientState���� �Լ������¼����
		//������Ϊ���ݴ����� ��TcpConnection����Ϣ����ص�����ΪNetManager��Ķ�Ӧ����
		++m_sessionId;
		std::shared_ptr<RoomSession> spSession(new RoomSession(conn, m_sessionId)); 
		conn->setMessageCallback(std::bind(&RoomSession::OnRead, spSession.get(), std::placeholders::_1, std::placeholders::_2));

		std::lock_guard<std::mutex> guard(m_sessionMutex);
		m_sessions.push_back(spSession);
	}
	else
	{
		OnClose(conn);
	}
}

void IMServer::OnClose(const std::shared_ptr<TcpConnection>& conn)
{
	//TODO Ϊ�� �����ǵ����ӶϿ��ǲ����Ͻ���Session���ͷ� �ȴ���������ʱ��ʹ��
	//���������ʵ�ֶ������� ��ֱ��ɾ��������RoomSession

	//˼������������������Session�ᱣ������ڷ�������״̬��������Ϸ��ʹ�Ͽ����ӣ����״̬Ҳ�ᱣ��һ��ʱ��
	//ֻ��Seesion�޷���ȡ�����ز������룬����������������������ݻ��ǻᴫ�͵��ˣ�������������Ӻ���ҽ�
	//��ȡ��Session�����ݻָ��ͻ��˵�״̬�Ӷ����¿�ʼ��Ϸ��
}
