#include "Buffer.h"
#include "Logger.h"
#include <string.h>
#include <unistd.h>
#include <assert.h>
#define CHARP_TO_INTP(x) ((int*)(x))
#define CHARP_TO_INTP_IDX0(x) CHARP_TO_INTP(x)[0]
const int HEAD_LEN = 4;

//Ӧ�ò�Э�飺������+���� ������Ϊ�����ݰ��ĳ��� ���ǽ�Ϊ���ݳ���
Buffer::Buffer()
{

}

Buffer::~Buffer()
{

}

bool Buffer::EnoughOnePackageLen()
{
	return all_data_len_ - current_index_ >=  solve_data_len_;
}

bool Buffer::EnoughHeadLen()
{
	return all_data_len_-current_index_>= HEAD_LEN;
}

void Buffer::SetSolveDataLen()
{
	solve_data_len_ = CHARP_TO_INTP_IDX0(datas_ + current_index_);
}

int Buffer::GetPackageLen()
{
	assert(EnoughOnePackageLen());
	return solve_data_len_;
}

int Buffer::GetDataLen()
{
	return all_data_len_;
}

void Buffer::SolveLen()
{
	current_index_ += solve_data_len_ ;
	if (current_index_ == all_data_len_)
	{
		current_index_ = 0;
		all_data_len_ = 0;
		return;
	}
	if (current_index_ > DATA_SIZE / 2)
	{
		memcpy(datas_, datas_ + current_index_, all_data_len_ - current_index_);
		all_data_len_ -= current_index_;
		current_index_ = 0;
	}
	LogInfo("current_index:%d,all_data_len:%d", current_index_, all_data_len_);
}

int Buffer::ReadData(int socket)
{
	int str_len = 0;
	while (true)
	{
		int read_len = read(socket, datas_ + all_data_len_, DATA_SIZE-all_data_len_);
		if (read_len > 0)
		{
			str_len += read_len;
			all_data_len_ += str_len;
		}
		else if(read_len==0)
		{
			break;		//����ر���socket  �����Ƴ�event�¼�ʱ�ᷢ������
		}
		else
		{
			//���ִ���
			if (errno != EAGAIN)
			{
				return read_len;
			}
			//û�д���
			break;
		}
	}
	return str_len;
}

char * Buffer::GetDatas()
{
	return datas_+current_index_;
}

void Buffer::AppendData(char * datas, int len)
{
	memcpy(datas_ + all_data_len_, datas, len);
	all_data_len_ += len;
}

bool Buffer::HasData()
{
	return current_index_!=all_data_len_;
}

void Buffer::MoveCurrentIndex(int index)
{
	current_index_ += index;
	if (current_index_ == all_data_len_)
	{
		current_index_ = 0;
		all_data_len_ = 0;
	}
}
