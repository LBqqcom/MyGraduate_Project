#include "MyThreadPool.h"
#include "EventLoop.h"
#include <assert.h>

MyThreadPool::MyThreadPool()
{

}

MyThreadPool::~MyThreadPool()
{
	//�ȴ������߳��˳�
	if (!is_stop_)
	{
		Stop();
	}
	//ɾ��������loop
	for (auto& loop : loops_)
	{
		delete loop;
	}
	//�����������
	tasks_.clear();
}

void MyThreadPool::Init(EventLoop* base_loop,int max_thread_count)
{
	//�߳��ѽ���
	assert(!is_stop_);

	current_use_thread_index_ = 0;
	is_stop_ = false;
	max_thread_count_ = max_thread_count < 1 ? 0 : max_thread_count;
	relaxed_thread_count_ = max_thread_count_;
	base_loop_ = base_loop;
}

void MyThreadPool::Start()
{
	//�߳��ѽ���
	assert(!is_stop_);
	//�߳�δ��ʼ��
	assert(base_loop_);
	for (int i = 0; i < max_thread_count_; i++)
	{
		threads_.emplace_back(std::bind(&MyThreadPool::Run, this));
	}
}

bool MyThreadPool::Run()
{
	while (!is_stop_)
	{
		EventLoop* loop = new EventLoop();
		{
			std::unique_lock<std::mutex> lk(mutx_);
			loops_.push_back(loop);
		}
		loop->loop();
		////�ȴ� �ж��˳� ִ�����
		//FunCallBack fcb;
		//{
		//	std::unique_lock<std::mutex> lk(mutx_);

		//	cond_.wait(lk, [&]()->bool {return !tasks_.empty() || is_stop_; });

		//	if (is_stop_)
		//	{
		//		break;
		//	}
		//	//fcb = std::move(tasks_.back());
		//	fcb = tasks_.back();
		//	tasks_.pop_back();
		//}
		//relaxed_thread_count_--;
		//fcb();
		//relaxed_thread_count_++;
	}
	return false;	//unable do this
}

//���Է�������
bool MyThreadPool::Stop()
{
	is_stop_ = true;
	cond_.notify_all();
	for (auto& t : threads_)
	{
		if (t.joinable())
		{
			t.join();
		}
	}
	return true;
}

EventLoop * MyThreadPool::GetNextLoop()
{
	//�߳�����ֹ
	assert(!is_stop_);
	//�߳�δ����
	assert(base_loop_);
	//���߳�����������; ������IO����
	if (max_thread_count_ == 0)
	{
		return base_loop_;
	}
	current_use_thread_index_++;
	if (current_use_thread_index_ == max_thread_count_)
	{
		current_use_thread_index_ = 0;
	}
	return loops_[current_use_thread_index_];
}


