#include"EventLoop.h"
#include "Logger.h"
#include "TimerQueue.h"
#include "Channel.h"
#include "EpollPoller.h"

#include <signal.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <assert.h>

namespace
{
	__thread EventLoop* t_loopInThisThread = 0;

	const int kPollTimeMs = 1;

	int createEventfd()
	{
		int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
		if (evtfd < 0)
		{
			LogInfo("Failed in eventfd");
			abort();
		}
		return evtfd;
	}

#pragma GCC diagnostic ignored "-Wold-style-cast"
	class IgnoreSigPipe
	{
	public:
		IgnoreSigPipe()
		{
			::signal(SIGPIPE, SIG_IGN);
			// LOG_TRACE << "Ignore SIGPIPE";
		}
	};
#pragma GCC diagnostic error "-Wold-style-cast"

	IgnoreSigPipe initObj;
}

EventLoop* EventLoop::getEventLoopOfCurrentThread()
{
	return t_loopInThisThread;
}

// ���̺߳����д���eventloop
EventLoop::EventLoop()
	: looping_(false),
	quit_(false),
	eventHandling_(false),
	callingPendingFunctors_(false),
	iteration_(0),
	threadId_(std::this_thread::get_id()),
	poller_(new EpollPoller(this)),
	timerQueue_(new TimerQueue(this)),
	wakeupFd_(createEventfd()),
	wakeupChannel_(new Channel(wakeupFd_, this)),
	currentActiveChannel_(NULL)
{
	if (t_loopInThisThread)
	{
		LogInfo("Another EventLoop  exists in this thread ");
	}
	else
	{
		t_loopInThisThread = this;
	}
	wakeupChannel_->SetReadCallBack(std::bind(&EventLoop::handleRead, this));
	// we are always reading the wakeupfd
	wakeupChannel_->enableReading();

	//std::stringstream ss;	
	//ss << "eventloop create threadid = " << threadId_;
	//std::cout << ss.str() << std::endl;
}

EventLoop::~EventLoop()
{
	assertInLoopThread();
	//LOG_DEBUG << "EventLoop destructs in other thread";

	//std::stringstream ss;
	//ss << "eventloop destructs threadid = " << threadId_;
	//std::cout << ss.str() << std::endl;

	wakeupChannel_->disableAll();
	wakeupChannel_->remove();
	::close(wakeupFd_);
	t_loopInThisThread = NULL;
}
//yys while��ѭ��
void EventLoop::loop()
{
	assert(!looping_);
	assertInLoopThread();
	looping_ = true;
	quit_ = false;  // FIXME: what if someone calls quit() before loop() ?
	LogInfo("EventLoop start looping");

	while (!quit_)
	{
		activeChannels_.clear();
		//�����¼� poller_->poll����epoll���������¼���Ȼ����Ŵ����������������¼���ÿһ���ͻ���\
		socket��Ӧһ�����ӣ���һ��TcpConnection��Channelͨ������
		poller_->poll(kPollTimeMs, &activeChannels_);
		++iteration_;
		eventHandling_ = true;
		for (ChannelList::iterator it = activeChannels_.begin();
			it != activeChannels_.end(); ++it)
		{
			currentActiveChannel_ = *it;
			//currentActiveChannel_->handleEvent(pollReturnTime_)�����ǿɶ�����д�������¼������ö�Ӧ��\
			����������Щ�������ǻص������������ʼ���׶����ý����ģ�
			currentActiveChannel_->SolveEvent();
		}
		currentActiveChannel_ = NULL;
		eventHandling_ = false;
		doPendingFunctors();
		//yys ��frameFunctor_�͸����ˣ�����ͨ������һ������ָ��Ϳ����ˡ���Ȼ�����и������ԵĶ�����\
		�����������ʱ��Ϊ���ܹ�����ִ�У�ʹ�û��ѻ��ƣ�ͨ����һ��fd����д��򵥵ļ����ֽڣ�������epoll��\
		ʹ�����̷��أ���Ϊ��ʱû��������socke���¼���������������ִ�иղ���ӵ������ˡ�
		if (frameFunctor_)
		{
			frameFunctor_();
		}
	}

	LogInfo("EventLoop stop looping");
	looping_ = false;
}
//�˳��߳�
void EventLoop::quit()
{
	quit_ = true;
	// There is a chance that loop() just executes while(!quit_) and exists,
	// then EventLoop destructs, then we are accessing an invalid object.
	// Can be fixed using mutex_ in both places.
	if (!isInLoopThread())
	{
		wakeup();
	}
}
//�������
void EventLoop::runInLoop(const Functor& cb)
{
	//����ǵ�ǰ�߳���ӵ���ֱ��ִ��
	if (isInLoopThread())
	{
		cb();
	}
	//������ӵ����������
	else
	{
		queueInLoop(cb);
	}
}
//yys ��������ĵط�	�����������̵߳���
void EventLoop::queueInLoop(const Functor& cb)
{
	//�������	���ܶ���̵߳���
	{
		std::unique_lock<std::mutex> lock(mutex_);
		pendingFunctors_.push_back(cb);
	}

	//���ǵ�ǰ�̻߳��߻�������ִ�ж��������ʱ����
	if (!isInLoopThread() || callingPendingFunctors_)
	{
		wakeup();
	}
}
//����֡�ص�����
void EventLoop::setFrameFunctor(const Functor& cb)
{
	frameFunctor_ = cb;
}
//�Ƴ�һ��channel
void EventLoop::removeChannel(Channel* channel)
{
	assert(channel->ownerLoop() == this);
	assertInLoopThread();
	if (eventHandling_)
	{
	/*	assert(currentActiveChannel_ == channel ||
			std::find(activeChannels_.begin(), activeChannels_.end(), channel) == activeChannels_.end());*/
	}

	LogInfo("Remove channel, channel fd = %d", channel->fd());
	poller_->removeChannel(channel);
}
//�ж��Ƿ����ĳ��channel
bool EventLoop::hasChannel(Channel* channel)
{
	assert(channel->ownerLoop() == this);
	assertInLoopThread();
	return poller_->hasChannel(channel);
}
//����ǵ�ǰ�߳�ִ��	����ĳЩѭ������ĺ��� ֻ���ڰ��������߳���ִ��	����������ٶ���
void EventLoop::abortNotInLoopThread()
{
	std::stringstream ss;
	ss << "threadid_ = " << threadId_ << " this_thread::get_id() = " << std::this_thread::get_id();
	LogInfo("EventLoop::abortNotInLoopThread - EventLoop %s",ss.str().c_str());
}
//д��һЩ���ݽ��̻߳���
void EventLoop::wakeup()
{
	uint64_t one = 1;
	ssize_t n = write(wakeupFd_, &one, sizeof one);
	if (n != sizeof one)
	{
		LogInfo("EventLoop::wakeup() writes %d bytes instead of 8",n);
	}
}
//��ȡ�����¼�д����ַ�
void EventLoop::handleRead()
{
	LogInfo("Loop wakuped by other thread");
	uint64_t one = 1;
	ssize_t n = read(wakeupFd_, &one, sizeof one);
	if (n != sizeof one)
	{
		LogInfo("EventLoop::handleRead() reads %d bytes instead of 8",n);
	}
}
//yys ��ѭ�������ҵ���߼������Ӧ��
void EventLoop::doPendingFunctors()
{
	std::vector<Functor> functors;
	callingPendingFunctors_ = true;

	{
		std::unique_lock<std::mutex> lock(mutex_);
		functors.swap(pendingFunctors_);
		/*
		��������ҵ���߼�������ִ������ĺ���ָ��ģ����ӵ����񱣴��ڳ�Ա����pendingFunctors_�У��������
		��һ������ָ�����飨vector���󣩣�ִ�е�ʱ�򣬵���ÿ�������Ϳ����ˡ�����Ĵ���������һ��ջ������
		��Ա����pendingFunctors_����ĺ���ָ�뻻�����������������ջ�������в����Ϳ����ˣ���������������
		���ȡ���Ϊ��Ա����pendingFunctors_�����������ʱ��Ҳ�ᱻ�õ�����Ƶ�����̲߳���������Ҫ����
		*/
	}//�˳���������  �����̴߳�ʱ�Ϳ��������߳����������

	for (size_t i = 0; i < functors.size(); ++i)
	{
		functors[i]();
	}
	callingPendingFunctors_ = false;
}