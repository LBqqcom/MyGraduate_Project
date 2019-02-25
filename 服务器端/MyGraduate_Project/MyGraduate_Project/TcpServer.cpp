#include "TcpServer.h"
#include "MyThreadPool.h"
#include "EventLoop.h"
#include "Logger.h"
#include "Global.h"
#include "Singleton.h"

#include <assert.h>

TcpServer::TcpServer(EventLoop* loop):started_(false),nextConnId_(1)
{
	assert(loop);
	loop_ = loop;
	acceptor_.reset(new Acceptor(loop_));
	acceptor_->SetNewConnectionCallBack(std::bind(&TcpServer::newConnection,this, std::placeholders::_1));
}

TcpServer::~TcpServer()
{
	loop_->assertInLoopThread();
	LogInfo ( "TcpServer::~TcpServer destructing");

	//�Ƴ���������
	for (TcpConnctions::iterator it(tcp_connections_.begin());it != tcp_connections_.end(); ++it)
	{
		TcpConnectionPtr conn = *it;
		(*it).reset();
		conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
		conn.reset();
	}
}

void TcpServer::Start()
{
	if (started_ == 0)
	{
		//threadPool_->start(threadInitCallback_);
		//assert(!acceptor_->listenning());
		////���ü�������
		loop_->runInLoop(std::bind(&Acceptor::Listen, acceptor_.get()));
		started_ = 1;
	}
}

void TcpServer::newConnection(int socket)
{
	EventLoop* ioLoop = Singleton<MyThreadPool>::Instance().GetNextLoop();
	TcpConnectionPtr new_connection(new TcpConnection(socket,ioLoop));
	//TODO	�Ƴ�player��connection��ֱ�����ӹ�ϵ
	new_connection->SetPlayer(lobby_.GetRoomSession());

	tcp_connections_.push_back(new_connection);
	new_connection->setConnectionCallback(connectionCallback_);	//IMServer::OnConnection
	new_connection->setMessageCallback(messageCallback_);
	new_connection->setWriteCompleteCallback(writeCompleteCallback_);	//û�ж�Ӧ�¼�  Ϊ��
	new_connection->setCloseCallback(std::bind(&TcpServer::removeConnection, this, std::placeholders::_1)); // FIXME: unsafe
	//���̷߳�����io�¼�����������TcpConnection::connectEstablished	-->IMServer::OnConnection
	ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, new_connection));
}

void TcpServer::removeConnection(const TcpConnectionPtr & conn)
{
	loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr & conn)
{
	loop_->assertInLoopThread();
	LogInfo("TcpServer::removeConnectionInLoop one connection");
	bool flag = false;
	for (auto tconn = tcp_connections_.begin(); tconn != tcp_connections_.end(); ++tconn)
	{
		if (*tconn == conn)
		{
			//ֻ�ǴӼ�����ɾ��  ��û���ͷ�
			tcp_connections_.erase(tconn);
			flag = true;
			break;
		}
	}
	if (!flag)
	{
		//���������������TcpConneaction�����ڴ��������У��Է��ͶϿ������ˡ�
		LogInfo("want to remove a connection ,but connection does not exist.");
		return;
	}
	EventLoop* ioLoop = conn->getLoop();
	//��Ȼ�ڱ���߳�ִ��
	ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}
