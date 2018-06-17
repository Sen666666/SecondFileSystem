

#include"inode.h"
#include"BufferManager.h"
#include "Buf.h"
#include"Kernel.h"
#include<algorithm>
#include<string.h>
#include<iostream>
#include <stdio.h>
using namespace std;





//======================================================INODE=========================================================//




/* 内存打开 i节点*/
myInode::myInode()
{
	/* 清空Inode对象中的数据 */
	// this->Clean(); 
	/* 去除this->Clean();的理由：
	* myInode::Clean()特定用于IAlloc()中清空新分配DiskInode的原有数据，
	* 即旧文件信息。Clean()函数中不应当清除i_dev, i_number, i_flag, i_count,
	* 这是属于内存Inode而非DiskInode包含的旧文件信息，而Inode类构造函数需要
	* 将其初始化为无效值。
	*/

	/* 将Inode对象的成员变量初始化为无效值 */
	this->i_flag = 0;
	this->i_mode = 0;
	this->i_count = 0;
	this->i_nlink = 0;
	this->i_dev = -1;
	this->i_number = -1;
	this->i_uid = -1;
	this->i_gid = -1;
	this->i_size = 0;
	this->i_lastr = -1;
	for (int i = 0; i < 10; i++)
	{
		this->i_addr[i] = 0;
	}
}



myInode::~myInode()
{
	//nothing to do here
}


void myInode::ReadI()
{
	int lbn;	/* 文件逻辑块号 */
	int bn;		/* lbn对应的物理盘块号 */
	int offset;	/* 当前字符块内起始传送位置 */
	int nbytes;	/* 传送至用户目标区字节数量 */
	short dev;
	myBuf* pBuf;
	myUser& u = myKernel::Instance().GetUser();
	myBufferManager& bufMgr = myKernel::Instance().GetBufferManager();

	if (0 == u.u_IOParam.m_Count)
	{
		/* 需要读字节数为零，则返回 */
		return;
	}

	this->i_flag |= myInode::IACC;

	/* 一次一个字符块地读入所需全部数据，直至遇到文件尾 */
	while (myUser::my_NOERROR == u.u_error && u.u_IOParam.m_Count != 0)
	{
		lbn = bn = u.u_IOParam.m_Offset / myInode::BLOCK_SIZE;
		offset = u.u_IOParam.m_Offset % myInode::BLOCK_SIZE;
		/* 传送到用户区的字节数量，取读请求的剩余字节数与当前字符块内有效字节数较小值 */
		nbytes = (myInode::BLOCK_SIZE - offset) < /* 块内有效字节数 */u.u_IOParam.m_Count ? (myInode::BLOCK_SIZE - offset) : u.u_IOParam.m_Count;

		if ((this->i_mode & myInode::IFMT) != myInode::IFBLK)
		{	/* 如果不是特殊块设备文件 */

			int remain = this->i_size - u.u_IOParam.m_Offset;
			/* 如果已读到超过文件结尾 */
			if (remain <= 0)
			{
				return;
			}
			/* 传送的字节数量还取决于剩余文件的长度 */
			nbytes = (nbytes < remain) ? nbytes : remain;

			/* 将逻辑块号lbn转换成物理盘块号bn ，Bmap有设置Inode::rablock。当UNIX认为获取预读块的开销太大时，
			* 会放弃预读，此时 myInode::rablock 值为 0。
			* */
			if ((bn = this->Bmap(lbn)) == 0)
			{
				return;
			}
			dev = this->i_dev;
		}
		else	/* 如果是特殊块设备文件 */ //注：永远不可能的啦
		{
			dev = this->i_addr[0];	/* 特殊块设备文件i_addr[0]中存放的是设备号 */
			//myInode::rablock = bn + 1;
		}

		//注：我这里先没有做预读操作，看有没有时间做
	
		pBuf = bufMgr.Bread(bn);
		
		/* 记录最近读取字符块的逻辑块号 */
		this->i_lastr = lbn;

		/* 缓存中数据起始读位置 */
		unsigned char* start = pBuf->b_addr + offset;

		//注：直接memcpy就好
		/* 读操作: 从缓冲区拷贝到用户目标区
		* i386芯片用同一张页表映射用户空间和内核空间，这一点硬件上的差异 使得i386上实现 iomove操作
		* 比PDP-11要容易许多*/
		//cout << "本次读入nbytes=" << nbytes <<"字节"<< endl;
		memcpy(u.u_IOParam.m_Base, start, nbytes);
		//Utility::IOMove(start, u.u_IOParam.m_Base, nbytes);

		/* 用传送字节数nbytes更新读写位置 */
		u.u_IOParam.m_Base += nbytes;
		u.u_IOParam.m_Offset += nbytes;
		u.u_IOParam.m_Count -= nbytes;

		bufMgr.Brelse(pBuf);	/* 使用完缓存，释放该资源 */
	}
	//cout << "ReadI返回" << endl;
}


void myInode::WriteI()
{
	//cout << "Inode.WriteI" << endl;
	int lbn;	/* 文件逻辑块号 */
	int bn;		/* lbn对应的物理盘块号 */
	int offset;	/* 当前字符块内起始传送位置 */
	int nbytes;	/* 传送字节数量 */
	short dev;
	myBuf* pBuf;
	myUser& u = myKernel::Instance().GetUser();
	myBufferManager& bufMgr = myKernel::Instance().GetBufferManager();

	/* 设置Inode被访问标志位 */
	this->i_flag |= (myInode::IACC | myInode::IUPD);

	if (0 == u.u_IOParam.m_Count)
	{
		//cout << "需要读写的字节数为0，WriteI直接返回" << endl;
		/* 需要读字节数为零，则返回 */
		return;
	}
	//cout << "需要写入的字节数为  u.u_IOParam.m_Count=" << u.u_IOParam.m_Count << endl;
	while (myUser::my_NOERROR == u.u_error && u.u_IOParam.m_Count != 0)
	{
		lbn = u.u_IOParam.m_Offset / myInode::BLOCK_SIZE;
		offset = u.u_IOParam.m_Offset % myInode::BLOCK_SIZE;
		nbytes = (myInode::BLOCK_SIZE - offset) < u.u_IOParam.m_Count ? (myInode::BLOCK_SIZE - offset) : u.u_IOParam.m_Count;

		if ((this->i_mode & myInode::IFMT) != myInode::IFBLK)
		{	/* 普通文件 */
			//cout << "本次写入普通文件" << endl;
			/* 将逻辑块号lbn转换成物理盘块号bn */
			if ((bn = this->Bmap(lbn)) == 0)
			{
				return;
			}
			dev = this->i_dev;
		}
		else
		{	/* 块设备文件，也就是硬盘 */
			//cout << "本次写入块设备文件？" << endl;
			dev = this->i_addr[0];
		}

		if (myInode::BLOCK_SIZE == nbytes)
		{
			/* 如果写入数据正好满一个字符块，则为其分配缓存 */
			pBuf = bufMgr.GetBlk( bn);
		}
		else
		{
			//cout << "本次写入不满一个字符块将进入bread，先读后写  物理盘块号码bn="<<bn << endl;
			/* 写入数据不满一个字符块，先读后写（读出该字符块以保护不需要重写的数据） */
			pBuf = bufMgr.Bread( bn);
		}

		/* 缓存中数据的起始写位置 */
		//cout << "缓存数据的起始写位置=pBuf->b_addr + offset  offset=" << offset << endl;
		unsigned char* start = pBuf->b_addr + offset;

		/* 写操作: 从用户目标区拷贝数据到缓冲区 */
		//cout << "写入数据为" << &u.u_IOParam.m_Base[4] <<"  nbytes="<<nbytes<< endl;
		memcpy(start, u.u_IOParam.m_Base, nbytes);
		//Utility::IOMove(u.u_IOParam.m_Base, start, nbytes);

		/* 用传送字节数nbytes更新读写位置 */
		u.u_IOParam.m_Base += nbytes;
		u.u_IOParam.m_Offset += nbytes;
		u.u_IOParam.m_Count -= nbytes;

		if (u.u_error != myUser::my_NOERROR)	/* 写过程中出错 */
		{
			cout << "writeI过程中出错,释放缓存" << endl;
			bufMgr.Brelse(pBuf);
		}
		else if ((u.u_IOParam.m_Offset % myInode::BLOCK_SIZE) == 0)	/* 如果写满一个字符块 */
		{
			/* 以异步方式将字符块写入磁盘，进程不需等待I/O操作结束，可以继续往下执行 */
			bufMgr.Bawrite(pBuf);
		}
		else /* 如果缓冲区未写满 */
		{
			/* 将缓存标记为延迟写，不急于进行I/O操作将字符块输出到磁盘上 */
			//cout << "注意：writeI标记的是延迟写" << endl;
			bufMgr.Bdwrite(pBuf);
		}

		/* 普通文件长度增加 */
		if ((this->i_size < u.u_IOParam.m_Offset) && (this->i_mode & (myInode::IFBLK & myInode::IFCHR)) == 0)
		{
			this->i_size = u.u_IOParam.m_Offset;
			//cout << "文件长度增加，现在为i_size=" << i_size << endl;
		}

		/*
		* 之前过程中读盘可能导致进程切换，在进程睡眠期间当前内存Inode可能
		* 被同步到外存Inode，在此需要重新设置更新标志位。
		* 好像没有必要呀！即使write系统调用没有上锁，iput看到i_count减到0之后才会将内存i节点同步回磁盘。而这在
		* 文件没有close之前是不会发生的。
		* 我们的系统对write系统调用上锁就更不可能出现这种情况了。
		* 真的想把它去掉。
		*/
		this->i_flag |= myInode::IUPD;
	}
	//cout << "WriteI返回" << endl;
}


int myInode::Bmap(int lbn)
{
	//cout << "Inode.Bmap" << endl;
	myBuf* pFirstBuf;
	myBuf* pSecondBuf;
	int phyBlkno;	/* 转换后的物理盘块号 */
	int* iTable;	/* 用于访问索引盘块中一次间接、两次间接索引表 */
	int index;
	myUser& u = myKernel::Instance().GetUser();
	myBufferManager& bufMgr = myKernel::Instance().GetBufferManager();
	myFileSystem& fileSys = myKernel::Instance().GetFileSystem();

	/*
	* Unix V6++的文件索引结构：(小型、大型和巨型文件)
	* (1) i_addr[0] - i_addr[5]为直接索引表，文件长度范围是0 - 6个盘块；
	*
	* (2) i_addr[6] - i_addr[7]存放一次间接索引表所在磁盘块号，每磁盘块
	* 上存放128个文件数据盘块号，此类文件长度范围是7 - (128 * 2 + 6)个盘块；
	*
	* (3) i_addr[8] - i_addr[9]存放二次间接索引表所在磁盘块号，每个二次间接
	* 索引表记录128个一次间接索引表所在磁盘块号，此类文件长度范围是
	* (128 * 2 + 6 ) < size <= (128 * 128 * 2 + 128 * 2 + 6)
	*/

	if (lbn >= myInode::HUGE_FILE_BLOCK)
	{
		u.u_error = myUser::my_EFBIG;
		return 0;
	}

	if (lbn < 6)		/* 如果是小型文件，从基本索引表i_addr[0-5]中获得物理盘块号即可 */
	{
		phyBlkno = this->i_addr[lbn];

		/*
		* 如果该逻辑块号还没有相应的物理盘块号与之对应，则分配一个物理块。
		* 这通常发生在对文件的写入，当写入位置超出文件大小，即对当前
		* 文件进行扩充写入，就需要分配额外的磁盘块，并为之建立逻辑块号
		* 与物理盘块号之间的映射。
		*/
		if (phyBlkno == 0 && (pFirstBuf = fileSys.Alloc()) != NULL)
		{
		//	cout << "物理盘块号为0，没有对应文件，此时已申请空闲的磁盘完毕,得到的磁盘块号为 pFirstBuf->b_blkno=" << pFirstBuf->b_blkno<< endl;
			/*
			* 因为后面很可能马上还要用到此处新分配的数据块，所以不急于立刻输出到
			* 磁盘上；而是将缓存标记为延迟写方式，这样可以减少系统的I/O操作。
			*/
			bufMgr.Bdwrite(pFirstBuf);
			phyBlkno = pFirstBuf->b_blkno;
			/* 将逻辑块号lbn映射到物理盘块号phyBlkno */
			this->i_addr[lbn] = phyBlkno;
			this->i_flag |= myInode::IUPD;
		}
		/* 找到预读块对应的物理盘块号 */
		//myInode::rablock = 0;
		if (lbn <= 4)
		{
			/*
			* i_addr[0] - i_addr[5]为直接索引表。如果预读块对应物理块号可以从
			* 直接索引表中获得，则记录在Inode::rablock中。如果需要额外的I/O开销
			* 读入间接索引块，就显得不太值得了。漂亮！
			*/
			//注：漂亮也没用 我没有预读
		//	myInode::rablock = this->i_addr[lbn + 1];
		}
	//	cout << "映射的物理盘块号为 phyBlkno=" << phyBlkno << "  Bmanp返回" << endl;
		return phyBlkno;
	}
	else	/* lbn >= 6 大型、巨型文件 */
	{
		/* 计算逻辑块号lbn对应i_addr[]中的索引 */

		if (lbn < myInode::LARGE_FILE_BLOCK)	/* 大型文件: 长度介于7 - (128 * 2 + 6)个盘块之间 */
		{
			index = (lbn - myInode::SMALL_FILE_BLOCK) / myInode::ADDRESS_PER_INDEX_BLOCK + 6;
		}
		else	/* 巨型文件: 长度介于263 - (128 * 128 * 2 + 128 * 2 + 6)个盘块之间 */
		{
			index = (lbn - myInode::LARGE_FILE_BLOCK) / (myInode::ADDRESS_PER_INDEX_BLOCK * myInode::ADDRESS_PER_INDEX_BLOCK) + 8;
		}

		phyBlkno = this->i_addr[index];
		/* 若该项为零，则表示不存在相应的间接索引表块 */
		if (0 == phyBlkno)
		{
			this->i_flag |= myInode::IUPD;
			/* 分配一空闲盘块存放间接索引表 */
			if ((pFirstBuf = fileSys.Alloc()) == NULL)
			{
				return 0;	/* 分配失败 */
			}
			/* i_addr[index]中记录间接索引表的物理盘块号 */
			this->i_addr[index] = pFirstBuf->b_blkno;
		}
		else
		{
			/* 读出存储间接索引表的字符块 */
			pFirstBuf = bufMgr.Bread( phyBlkno);
		}
		/* 获取缓冲区首址 */
		iTable = (int *)pFirstBuf->b_addr;

		if (index >= 8)	/* ASSERT: 8 <= index <= 9 */
		{
			/*
			* 对于巨型文件的情况，pFirstBuf中是二次间接索引表，
			* 还需根据逻辑块号，经由二次间接索引表找到一次间接索引表
			*/
			index = ((lbn - myInode::LARGE_FILE_BLOCK) / myInode::ADDRESS_PER_INDEX_BLOCK) % myInode::ADDRESS_PER_INDEX_BLOCK;

			/* iTable指向缓存中的二次间接索引表。该项为零，不存在一次间接索引表 */
			phyBlkno = iTable[index];
			if (0 == phyBlkno)
			{
				if ((pSecondBuf = fileSys.Alloc()) == NULL)
				{
					/* 分配一次间接索引表磁盘块失败，释放缓存中的二次间接索引表，然后返回 */
					bufMgr.Brelse(pFirstBuf);
					return 0;
				}
				/* 将新分配的一次间接索引表磁盘块号，记入二次间接索引表相应项 */
				iTable[index] = pSecondBuf->b_blkno;
				/* 将更改后的二次间接索引表延迟写方式输出到磁盘 */
				bufMgr.Bdwrite(pFirstBuf);
			}
			else
			{
				/* 释放二次间接索引表占用的缓存，并读入一次间接索引表 */
				bufMgr.Brelse(pFirstBuf);
				pSecondBuf = bufMgr.Bread( phyBlkno);
			}

			pFirstBuf = pSecondBuf;
			/* 令iTable指向一次间接索引表 */
			iTable = (int *)pSecondBuf->b_addr;
		}

		/* 计算逻辑块号lbn最终位于一次间接索引表中的表项序号index */

		if (lbn < myInode::LARGE_FILE_BLOCK)
		{
			index = (lbn - myInode::SMALL_FILE_BLOCK) % myInode::ADDRESS_PER_INDEX_BLOCK;
		}
		else
		{
			index = (lbn - myInode::LARGE_FILE_BLOCK) % myInode::ADDRESS_PER_INDEX_BLOCK;
		}

		if ((phyBlkno = iTable[index]) == 0 && (pSecondBuf = fileSys.Alloc()) != NULL)
		{
			/* 将分配到的文件数据盘块号登记在一次间接索引表中 */
			phyBlkno = pSecondBuf->b_blkno;
			iTable[index] = phyBlkno;
			/* 将数据盘块、更改后的一次间接索引表用延迟写方式输出到磁盘 */
			bufMgr.Bdwrite(pSecondBuf);
			bufMgr.Bdwrite(pFirstBuf);
		}
		else
		{
			/* 释放一次间接索引表占用缓存 */
			bufMgr.Brelse(pFirstBuf);
		}
		/* 找到预读块对应的物理盘块号，如果获取预读块号需要额外的一次for间接索引块的IO，不合算，放弃 */
		//myInode::rablock = 0;
		if (index + 1 < myInode::ADDRESS_PER_INDEX_BLOCK)
		{
			//myInode::rablock = iTable[index + 1];
		}
		return phyBlkno;
	}
}

void myInode::IUpdate()
{
	myBuf* pBuf;
	DiskInode dInode;
	myFileSystem& filesys = myKernel::Instance().GetFileSystem();
	myBufferManager& bufMgr = myKernel::Instance().GetBufferManager();

	/* 当IUPD和IACC标志之一被设置，才需要更新相应DiskInode
	* 目录搜索，不会设置所途径的目录文件的IACC和IUPD标志 */
	if ((this->i_flag & (myInode::IUPD | myInode::IACC)) != 0)
	{
		if (filesys.GetFS()->s_ronly != 0)
		{
			/* 如果该文件系统只读 */
			return;
		}

		/* 邓蓉的注释：在缓存池中找到包含本i节点（this->i_number）的缓存块
		* 这是一个上锁的缓存块，本段代码中的Bwrite()在将缓存块写回磁盘后会释放该缓存块。
		* 将该存放该DiskInode的字符块读入缓冲区 */
		pBuf = bufMgr.Bread(myFileSystem::INODE_ZONE_START_SECTOR + this->i_number / myFileSystem::INODE_NUMBER_PER_SECTOR);

		/* 将内存Inode副本中的信息复制到dInode中，然后将dInode覆盖缓存中旧的外存Inode */
		dInode.d_mode = this->i_mode;
		dInode.d_nlink = this->i_nlink;
		dInode.d_uid = this->i_uid;
		dInode.d_gid = this->i_gid;
		dInode.d_size = this->i_size;
		for (int i = 0; i < 10; i++)
		{
			dInode.d_addr[i] = this->i_addr[i];
		}


		//注：本次课设不需要更新时间
		if (this->i_flag & myInode::IACC)
		{
			/* 更新最后访问时间 */
			//dInode.d_atime = time;
		}
		if (this->i_flag & myInode::IUPD)
		{
			/* 更新最后访问时间 */
			//dInode.d_mtime = time;
		}

		//注：这代码写得 转成char*又转成int*？？
		/* 将p指向缓存区中旧外存Inode的偏移位置 */
		unsigned char* p = pBuf->b_addr + (this->i_number % myFileSystem::INODE_NUMBER_PER_SECTOR) * sizeof(DiskInode);
		DiskInode* pNode = &dInode;

		/* 用dInode中的新数据覆盖缓存中的旧外存Inode */
		memcpy(p, pNode, sizeof(DiskInode));
		// Utility::DWordCopy((int *)pNode, (int *)p, sizeof(DiskInode) / sizeof(int));

		/* 将缓存写回至磁盘，达到更新旧外存Inode的目的 */
		bufMgr.Bwrite(pBuf);
	}
}



void myInode::ITrunc()
{
	/* 经由磁盘高速缓存读取存放一次间接、两次间接索引表的磁盘块 */
	myBufferManager& bm = myKernel::Instance().GetBufferManager();
	/* 获取g_FileSystem对象的引用，执行释放磁盘块的操作 */
	myFileSystem& filesys = myKernel::Instance().GetFileSystem();

	
	/* 如果是字符设备或者块设备则退出 */
	if (this->i_mode & (myInode::IFCHR & myInode::IFBLK))
	{
		printf("in itrunc in IFCHR or IFBLK quit\n");
		return;
	}

	/* 采用FILO方式释放，以尽量使得SuperBlock中记录的空闲盘块号连续。
	*
	* Unix V6++的文件索引结构：(小型、大型和巨型文件)
	* (1) i_addr[0] - i_addr[5]为直接索引表，文件长度范围是0 - 6个盘块；
	*
	* (2) i_addr[6] - i_addr[7]存放一次间接索引表所在磁盘块号，每磁盘块
	* 上存放128个文件数据盘块号，此类文件长度范围是7 - (128 * 2 + 6)个盘块；
	*
	* (3) i_addr[8] - i_addr[9]存放二次间接索引表所在磁盘块号，每个二次间接
	* 索引表记录128个一次间接索引表所在磁盘块号，此类文件长度范围是
	* (128 * 2 + 6 ) < size <= (128 * 128 * 2 + 128 * 2 + 6)
	*/
	for (int i = 9; i >= 0; i--)		/* 从i_addr[9]到i_addr[0] */
	{
		/* 如果i_addr[]中第i项存在索引 */
		if (this->i_addr[i] != 0)
		{
			/* 如果是i_addr[]中的一次间接、两次间接索引项 */
			if (i >= 6 && i <= 9)
			{
				/* 将间接索引表读入缓存 */
				myBuf* pFirstBuf = bm.Bread(this->i_addr[i]);
				/* 获取缓冲区首址 */
				int* pFirst = (int *)pFirstBuf->b_addr;

				/* 每张间接索引表记录 512/sizeof(int) = 128个磁盘块号，遍历这全部128个磁盘块 */
				for (int j = 128 - 1; j >= 0; j--)
				{
					if (pFirst[j] != 0)	/* 如果该项存在索引 */
					{
						/*
						* 如果是两次间接索引表，i_addr[8]或i_addr[9]项，
						* 那么该字符块记录的是128个一次间接索引表存放的磁盘块号
						*/
						if (i >= 8 && i <= 9)
						{
							myBuf* pSecondBuf = bm.Bread(pFirst[j]);
							int* pSecond = (int *)pSecondBuf->b_addr;

							for (int k = 128 - 1; k >= 0; k--)
							{
								if (pSecond[k] != 0)
								{
									/* 释放指定的磁盘块 */
									filesys.Free( pSecond[k]);
								}
							}
							/* 缓存使用完毕，释放以便被其它进程使用 */
							bm.Brelse(pSecondBuf);
						}
						filesys.Free( pFirst[j]);
					}
				}
				bm.Brelse(pFirstBuf);
			}
			/* 释放索引表本身占用的磁盘块 */
			filesys.Free( this->i_addr[i]);
			/* 0表示该项不包含索引 */
			this->i_addr[i] = 0;
		}
	}

	/* 盘块释放完毕，文件大小清零 */
	this->i_size = 0;
	/* 增设IUPD标志位，表示此内存Inode需要同步到相应外存Inode */
	this->i_flag |= myInode::IUPD;
	/* 清大文件标志 和原来的RWXRWXRWX比特*/
	this->i_mode &= ~(myInode::ILARG & myInode::IRWXU & myInode::IRWXG & myInode::IRWXO);
	this->i_nlink = 1;
}



void myInode::Clean()
{
	/*
	* myInode::Clean()特定用于IAlloc()中清空新分配DiskInode的原有数据，
	* 即旧文件信息。Clean()函数中不应当清除i_dev, i_number, i_flag, i_count,
	* 这是属于内存Inode而非DiskInode包含的旧文件信息，而Inode类构造函数需要
	* 将其初始化为无效值。
	*/

	// this->i_flag = 0;
	this->i_mode = 0;
	//this->i_count = 0;
	this->i_nlink = 0;
	//this->i_dev = -1;
	//this->i_number = -1;
	this->i_uid = -1;
	this->i_gid = -1;
	this->i_size = 0;
	this->i_lastr = -1;
	for (int i = 0; i < 10; i++)
	{
		this->i_addr[i] = 0;
	}
}


void myInode::ICopy(myBuf *bp, int inumber)
{
	//cout << "Inode.ICopy" << endl;
	DiskInode dInode;
	DiskInode* pNode = &dInode;

	/* 将p指向缓存区中编号为inumber外存Inode的偏移位置 */
	unsigned char* p = bp->b_addr + (inumber % myFileSystem::INODE_NUMBER_PER_SECTOR) * sizeof(DiskInode);
	/* 将缓存中外存Inode数据拷贝到临时变量dInode中，按4字节拷贝 */
	//注；我在这里突然觉得按四字节拷贝可能是为了和带宽相契合
	//cout << "将缓存中的DiskInode拷贝到一个中间变量 inode在缓存中的序号" << (inumber % myFileSystem::INODE_NUMBER_PER_SECTOR) * sizeof(DiskInode) << endl;
	memcpy(pNode, p, sizeof(DiskInode));
	//Utility::DWordCopy((int *)p, (int *)pNode, sizeof(DiskInode) / sizeof(int));

	/* 将外存Inode变量dInode中信息复制到内存Inode中 */
	//cout << "这个DiskInode-d_mode=" << dInode.d_mode<<"  DiskInode.d_nlink=" <<dInode.d_nlink<< endl;
	this->i_mode = dInode.d_mode;
	this->i_nlink = dInode.d_nlink;
	this->i_uid = dInode.d_uid;
	this->i_gid = dInode.d_gid;
	this->i_size = dInode.d_size;
	for (int i = 0; i < 10; i++)
	{
		this->i_addr[i] = dInode.d_addr[i];
	}
	//cout << "Icopy成功返回" << endl;
}



DiskInode::DiskInode()
{
	/*
	* 如果DiskInode没有构造函数，会发生如下较难察觉的错误：
	* DiskInode作为局部变量占据函数Stack Frame中的内存空间，但是
	* 这段空间没有被正确初始化，仍旧保留着先前栈内容，由于并不是
	* DiskInode所有字段都会被更新，将DiskInode写回到磁盘上时，可能
	* 将先前栈内容一同写回，导致写回结果出现莫名其妙的数据。
	*/
	this->d_mode = 0;
	this->d_nlink = 0;
	this->d_uid = -1;
	this->d_gid = -1;
	this->d_size = 0;
	for (int i = 0; i < 10; i++)
	{
		this->d_addr[i] = 0;
	}
	this->d_atime = 0;
	this->d_mtime = 0;
}

DiskInode::~DiskInode()
{
	//nothing to do here
}
