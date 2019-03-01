#ifndef __BUFFER_H__
#define __BUFFER_H__
const int DATA_SIZE = 1024;
class Buffer
{
public:
	Buffer();
	~Buffer();
	bool RsEnoughOnePackageLen();
	bool RsEnoughHeadLen();
	void SetSolveDataLen();
	int GetPackageLen();
	int GetValidDataLen();
	void SolveLen();
	int ReadData(int socket);
	char* GetDatas();
	void AppendData(char *datas, int len);
	bool HasData();
	void MoveCurrentIndex(int index);
private:
	char datas_[DATA_SIZE];
	int current_index_ = 0;		//��ǰ������ʵλ��
	int solve_data_len_ = 0;	//�������ݳ���	
	int all_data_len_ = 0;		//���������ܳ���
};
#endif // !__BUFFER_H__
