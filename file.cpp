

#include"file.h"
#include"user.h"
#include"Kernel.h"
#include<iostream>
#include <stdio.h>

using namespace std;

/*==============================class File===================================*/
myFile::myFile()
{
	this->f_count = 0;
	this->f_flag = 0;
	this->f_offset = 0;
	this->f_inode = NULL;
}

myFile::~myFile()
{
	//nothing to do here
}

/*==============================class OpenFiles===================================*/
myOpenFiles::myOpenFiles()
{
}

myOpenFiles::~myOpenFiles()
{
}

int myOpenFiles::AllocFreeSlot()
{
//	cout << "OpenFile.AllicFreeSlot" << endl;
	int i;
	myUser& u = myKernel::Instance().GetUser();

	for (i = 0; i < myOpenFiles::NOFILES; i++)
	{
		/* 进程打开文件描述符表中找到空闲项，则返回之 */
		if (this->ProcessOpenFileTable[i] == NULL)
		{
			/* 设置核心栈现场保护区中的EAX寄存器的值，即系统调用返回值 */
	//		cout << "分配到相应的File结构，返回FIle块的索引号为 i=" << i << " 且置u.u_ar0为i  AllocFreeSlot成功返回" << endl;
			u.u_ar0 = i;
			return i;
		}
	}
//	cout << "未分配到空闲的File块，AllocFreeSlot错误返回-1，u.u_ar0=-1" << endl;
	u.u_ar0 = -1;   /* Open1，需要一个标志。当打开文件结构创建失败时，可以回收系统资源*/
	u.u_error = myUser::my_EMFILE;
	return -1;
}


myFile* myOpenFiles::GetF(int fd)
{
//	cout << "OpenFiles.GetF fd=" << fd << endl;
	myFile* pFile;
	myUser& u = myKernel::Instance().GetUser();

	/* 如果打开文件描述符的值超出了范围 */
	if (fd < 0 || fd >= myOpenFiles::NOFILES)
	{
		u.u_error = myUser::my_EBADF;
		return NULL;
	}

	pFile = this->ProcessOpenFileTable[fd];
	if (pFile == NULL)
	{
		cout << "没有找到fd对应的File结构,GetF错误返回" << endl;
		u.u_error = myUser::my_EBADF;
	}
//	cout << "GetF 函数返回" << endl;
	return pFile;	/* 即使pFile==NULL也返回它，由调用GetF的函数来判断返回值 */
}


void myOpenFiles::SetF(int fd, myFile* pFile)
{
	//cout << "OpenFiles.SetF" << endl;
	if (fd < 0 || fd >= myOpenFiles::NOFILES)
	{
		cout << "参数错误，SetF错误返回" << endl;
		return;
	}
	/* 进程打开文件描述符指向系统打开文件表中相应的File结构 */
//	cout << "成功建立fd与File块的勾连关系，SetF正确返回" << endl;
	this->ProcessOpenFileTable[fd] = pFile;
}

/*==============================class IOParameter===================================*/
IOParameter::IOParameter()
{
	this->m_Base = 0;
	this->m_Count = 0;
	this->m_Offset = 0;
}

IOParameter::~IOParameter()
{
	//nothing to do here
}

