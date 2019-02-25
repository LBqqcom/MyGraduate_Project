#include "EpollPoller.h"
#include "Logger.h"
#include "Global.h"

#include <sys/epoll.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <sstream>

namespace
{
	const int kNew = -1;		//�µ�channel
	const int kAdded = 1;		//�Ѿ���ӵ�channel
	const int kDeleted = 2;		//׼����ɾ����channel
}

EpollPoller::EpollPoller(EventLoop* loop)
	:events_(kInitEventListSize),
	ownerLoop_(loop)
{
	epollfd_ = epoll_create(256);
	if (epollfd_ < 0)
	{
		LogInfo("EpollPoller::EpollPoller");
	}
}

EpollPoller::~EpollPoller()
{
	::close(epollfd_);
}
//�ж��Ƿ����channel
bool EpollPoller::hasChannel(Channel* channel) const
{
	assertInLoopThread();
	ChannelMap::const_iterator it = channels_.find(channel->fd());
	return it != channels_.end() && it->second == channel;
}

void EpollPoller::poll(int timeoutMs, ChannelList* activeChannels)
{
	int numEvents = ::epoll_wait(epollfd_,&*events_.begin(),static_cast<int>(events_.size()),timeoutMs);

	int savedErrno = errno;
	if (numEvents > 0)
	{
		std::stringstream ss;
		ss << "this_thread::get_id() = " << std::this_thread::get_id();
		LogInfo("%d events happended %s", numEvents, ss.str().c_str());
		fillActiveChannels(numEvents, activeChannels);
		//����Ԥ����¼�����
		if (static_cast<size_t>(numEvents) == events_.size())
		{
			events_.resize(events_.size() * 2);
		}
	}
	else if (numEvents == 0)
	{
		//LOG_TRACE << " nothing happended";
	}
	else
	{
		// error happens, log uncommon ones
		if (savedErrno != EINTR)
		{
			errno = savedErrno;
			LogInfo("EpollPoller::poll()");
		}
	}
}
//���channel
void EpollPoller::fillActiveChannels(int numEvents,ChannelList* activeChannels) const
{
	assert( numEvents <= events_.size());
	for (int i = 0; i < numEvents; ++i)
	{
		Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
		//���ý����¼�
		channel->set_revents(events_[i].events);	
		activeChannels->push_back(channel);
	}
}
//��channel����event����
bool EpollPoller::updateChannel(Channel* channel)
{
	assertInLoopThread();
	LogInfo("updateChannel fd = %d events = %d",channel->fd(),channel->events());
	const int index = channel->index();
	//û�д����¼� ������Ӳ���
	int fd = channel->fd();
	if (index == kNew || index == kDeleted)
	{
		// a new one, add with EPOLL_CTL_ADD
		if (index == kNew)
		{
			//assert(channels_.find(fd) == channels_.end())
			if (channels_.find(fd) != channels_.end())
			{
				LogInfo( "fd = %d must not exist in channels_",fd);
				return false;
			}
			//��channel���뵽map��
			channels_[fd] = channel;
		}
		else // index == kDeleted
		{
			//������
			if (channels_.find(fd) == channels_.end())
			{
				LogInfo("fd = %d must exist in channels_", fd);
				return false;
			}

			//��ƥ��
			if (channels_[fd] != channel)
			{
				LogInfo("current channel is not matched current fd, fd =", fd);
				return false;
			}
		}
		channel->set_index(kAdded);

		return update(EPOLL_CTL_ADD, channel);
	}
	//�����¼� ����ɾ��/�޸Ĳ���
	else
	{
		// update existing one with EPOLL_CTL_MOD/DEL
		//assert(channels_.find(fd) != channels_.end());
		//assert(channels_[fd] == channel);
		//assert(index == kAdded);
		//������ ��ƥ��channel ��ƥ���¼�
		if (channels_.find(fd) == channels_.end() || channels_[fd] != channel || index != kAdded)
		{
			LogInfo("current channel is not matched current fd, fd = %d channel = ", fd);
			return false;
		}
		//�ж��Ƿ񲻴����¼�
		if (channel->isNoneEvent())
		{
			//���������Ƴ��¼� �����channelΪ��ɾ����
			if (update(EPOLL_CTL_DEL, channel))
			{
				channel->set_index(kDeleted);
				return true;
			}
			return false;
		}
		//�������޸�
		else
		{
			return update(EPOLL_CTL_MOD, channel);
		}
	}
}
//�Ƴ�channel
void EpollPoller::removeChannel(Channel* channel)
{
	assertInLoopThread();
	int fd = channel->fd();
	LogInfo("remove channel fd =%d",fd);
	assert(channels_.find(fd) != channels_.end());		//������
	assert(channels_[fd] == channel);					//��ƥ��
	assert(channel->isNoneEvent());						//�����¼�
	int index = channel->index();
	assert(index == kAdded || index == kDeleted);		//������ӵĻ���ɾ����
	size_t n = channels_.erase(fd);						//��map��ɾ��channel
	(void)n;
	assert(n == 1);

	if (index == kAdded)								//����¼�
	{
		update(EPOLL_CTL_DEL, channel);
	}
	//����Ϊ�µ�channel
	channel->set_index(kNew);
}

//yys ����EPOLL�¼�
bool EpollPoller::update(int operation, Channel* channel)
{
	struct epoll_event event;
	bzero(&event, sizeof event);
	event.events = channel->events();
	event.data.ptr = channel;
	int fd = channel->fd();
	if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
	{
		LogInfo("epoll_ctl op=%d fd=%d, epollfd=%d, errno=%d, errorInfo:%s", operation, fd, epollfd_, errno, strerror(errno));
		return false;
	}
	return true;
}
