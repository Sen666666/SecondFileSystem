

#include"openfilemanager.h"
#include"user.h"
#include"Kernel.h"


#include<iostream>
#include <stdio.h>
using namespace std;
//注：全局superblock实例
mySuperBlock g_spb;
/*==============================class OpenFileTable===================================*/
/* 系统全局打开文件表对象实例的定义 */
myOpenFileTable g_OpenFileTable;

myOpenFileTable::myOpenFileTable()
{
	//nothing to do here
}

myOpenFileTable::~myOpenFileTable()
{
	//nothing to do here
}

/*作用：进程打开文件描述符表中找的空闲项  之 下标  写入 u_ar0[EAX]*/
myFile* myOpenFileTable::FAlloc()
{
	//cout << "OpenFileTable.FAlloc" << endl;
	int fd;
	myUser& u = myKernel::Instance().GetUser();

	/* 在进程打开文件描述符表中获取一个空闲项 */
	fd = u.u_ofiles.AllocFreeSlot();

	if (fd < 0)	/* 如果寻找空闲项失败 */
	{
		//cout << "未寻找到空闲File块 FAlloc返回NULL" << endl;
		return NULL;
	}

	for (int i = 0; i < myOpenFileTable::NFILE; i++)
	{
		/* f_count==0表示该项空闲 */
		if (this->m_File[i].f_count == 0)
		{
			/* 建立描述符和File结构的勾连关系 */
			u.u_ofiles.SetF(fd, &this->m_File[i]);
			/* 增加对file结构的引用计数 */
			this->m_File[i].f_count++;
			/* 清空文件读、写位置 */
			this->m_File[i].f_offset = 0;
			//cout << "FAlloc 正确返回对应分配的File块" << endl;
			return (&this->m_File[i]);
		}
	}
	printf("No Free File Struct\n");
	//Diagnose::Write("No Free File Struct\n");
	u.u_error = myUser::my_ENFILE;
	return NULL;
}


void myOpenFileTable::CloseF(myFile *pFile)
{
	myInode* pNode;

	//注：不会是管道类型

	if (pFile->f_count <= 1)
	{
		/*
		* 如果当前进程是最后一个引用该文件的进程，
		* 对特殊块设备、字符设备文件调用相应的关闭函数
		*/
		//注：不需要调用设备的关闭函数，释放相应inode即可
	//	pFile->f_inode->CloseI(pFile->f_flag & File::FWRITE);
		g_InodeTable.IPut(pFile->f_inode);
	}

	/* 引用当前File的进程数减1 */
	pFile->f_count--;
}


/*==============================class myInodeTable===================================*/
/*  定义内存Inode表的实例 */
myInodeTable g_InodeTable;

myInodeTable::myInodeTable()
{
	//nothing to do here
}

myInodeTable::~myInodeTable()
{
	//nothing to do here
}

void myInodeTable::Initialize()
{
	/* 获取对g_FileSystem的引用 */
	this->m_FileSystem = &myKernel::Instance().GetFileSystem();
}

myInode* myInodeTable::IGet( int inumber)
{

	//cout << "InodeTable.IGet" << endl;
	myInode* pInode;
	myUser& u = myKernel::Instance().GetUser();

	while (true)
	{
		/* 检查指定设备dev中编号为inumber的外存Inode是否有内存拷贝 */
		int index = this->IsLoaded( inumber);
		if (index >= 0)	/* 找到内存拷贝 */
		{
			pInode = &(this->m_Inode[index]);
			/* 如果该内存Inode被上锁 */
			//注：不存在锁

			/* 如果该内存Inode用于连接子文件系统，查找该Inode对应的Mount装配块 */
			//注：不存在子文件系统的情况

			/*
			* 程序执行到这里表示：内存Inode高速缓存中找到相应内存Inode，
			* 增加其引用计数，增设ILOCK标志并返回之
			*/
			pInode->i_count++;
			//cout << "该inode已经被loaded将会返回该结点 该inode为"<<inumber <<"Iget函数返回"<< endl;
			return pInode;
		}
		else	/* 没有Inode的内存拷贝，则分配一个空闲内存Inode */
		{
			pInode = this->GetFreeInode();
			/* 若内存Inode表已满，分配空闲Inode失败 */
			if (NULL == pInode)
			{
				printf("Inode Table Overflow !\n");
				//Diagnose::Write("Inode Table Overflow !\n");
				u.u_error = myUser::my_ENFILE;
				return NULL;
			}
			else	/* 分配空闲Inode成功，将外存Inode读入新分配的内存Inode */
			{
				/* 设置新的设备号、外存Inode编号，增加引用计数，对索引节点上锁 */
				//pInode->i_dev = dev;
				pInode->i_number = inumber;
			//	cout << "成功获得空闲inode pInode->i_number=" << pInode->i_number << endl;
				pInode->i_count++;
				pInode->i_lastr = -1;

				myBufferManager& bm = myKernel::Instance().GetBufferManager();
				/* 将该外存Inode读入缓冲区 */
				myBuf* pBuf = bm.Bread(myFileSystem::INODE_ZONE_START_SECTOR + inumber / myFileSystem::INODE_NUMBER_PER_SECTOR);

				/* 如果发生I/O错误 */
				if (pBuf->b_flags & myBuf::B_ERROR)
				{
					cout << "发生缓存错误，释放缓存，IGet将错误返回" << endl;
					/* 释放缓存 */
					bm.Brelse(pBuf);
					/* 释放占据的内存Inode */
					this->IPut(pInode);
					return NULL;
				}

				/* 将缓冲区中的外存Inode信息拷贝到新分配的内存Inode中 */
				pInode->ICopy(pBuf, inumber);
				/* 释放缓存 */
				bm.Brelse(pBuf);
			//	cout << "释放完buf，Iget将成功返回" << endl;
				return pInode;
			}
		}
	}
	return NULL;	/* GCC likes it! */
}

/* close文件时会调用Iput
*      主要做的操作：内存i节点计数 i_count--；若为0，释放内存 i节点、若有改动写回磁盘
* 搜索文件途径的所有目录文件，搜索经过后都会Iput其内存i节点。路径名的倒数第2个路径分量一定是个
*   目录文件，如果是在其中创建新文件、或是删除一个已有文件；再如果是在其中创建删除子目录。那么
*   	必须将这个目录文件所对应的内存 i节点写回磁盘。
*   	这个目录文件无论是否经历过更改，我们必须将它的i节点写回磁盘。
* */
void myInodeTable::IPut(myInode *pNode)
{
	//cout << "Inodetable.IPut" << endl;
	/* 当前进程为引用该内存Inode的唯一进程，且准备释放该内存Inode */
	if (pNode->i_count == 1)
	{

		/* 该文件已经没有目录路径指向它 */
		if (pNode->i_nlink <= 0)
		{
			/* 释放该文件占据的数据盘块 */
			pNode->ITrunc();
			pNode->i_mode = 0;
			/* 释放对应的外存Inode */
			this->m_FileSystem->IFree(pNode->i_number);
		}

		/* 更新外存Inode信息 */
		pNode->IUpdate();

		/* 清除内存Inode的所有标志位 */
		pNode->i_flag = 0;
		/* 这是内存inode空闲的标志之一，另一个是i_count == 0 */
		pNode->i_number = -1;
	}

	/* 减少内存Inode的引用计数，唤醒等待进程 */
	pNode->i_count--;
}

void myInodeTable::UpdateInodeTable()
{
	for (int i = 0; i < myInodeTable::NINODE; i++)
	{
		/*
		* 如果Inode对象没有被上锁，即当前未被其它进程使用，可以同步到外存Inode；
		* 并且count不等于0，count == 0意味着该内存Inode未被任何打开文件引用，无需同步。
		*/
		if ( this->m_Inode[i].i_count != 0)
		{
			/* 将内存Inode上锁后同步到外存Inode */
			this->m_Inode[i].i_flag |= myInode::ILOCK;
			this->m_Inode[i].IUpdate();
		}
	}
}

int myInodeTable::IsLoaded( int inumber)
{
	//cout << "InodeTable.IsLoaded" << endl;
	/* 寻找指定外存Inode的内存拷贝 */
	for (int i = 0; i < myInodeTable::NINODE; i++)
	{
		if ( this->m_Inode[i].i_number == inumber && this->m_Inode[i].i_count != 0)
		{
		//	cout << "找到相应内存inode节点，编号为" <<i<< endl;
			return i;
		}
	}
//	cout << "没找到相应内存inode节点"<< endl;
	return -1;
}

myInode* myInodeTable::GetFreeInode()
{
	//cout << "in GetFreeInode" << endl;
	for (int i = 0; i < myInodeTable::NINODE; i++)
	{
		/* 如果该内存Inode引用计数为零，则该Inode表示空闲 */
		if (this->m_Inode[i].i_count == 0)
		{
		//	cout << "将返回空闲内存inode地址" << endl;
			return &(this->m_Inode[i]);
		}
	}
	cout << "寻找空闲内存inode失败返回NULL" << endl;
	return NULL;	/* 寻找失败 */
}
