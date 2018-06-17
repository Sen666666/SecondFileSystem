#include "BufferManager.h"
#include "Kernel.h"
//#include"BlockDevice.h"
#include<string.h>
#include<iostream>


using namespace std;
myBufferManager::myBufferManager()
{
	//nothing to do here
}

myBufferManager::~myBufferManager()
{
	//nothing to do here
}

void myBufferManager::Initialize(char *start)
{
	//printf("in mybuffermanager ini\n");
	this->p = start;
	int i;
	myBuf* bp;

	this->bFreeList.b_forw = this->bFreeList.b_back = &(this->bFreeList);

	for (i = 0; i < NBUF; i++)
	{
		bp = &(this->m_Buf[i]);
		bp->b_addr = this->Buffer[i];
		/* 初始化NODEV队列 */
		//注：NODEV是一个特殊的设备,表示无设备
		//注：这代码写的真的太强了coool！！！
		bp->b_back = &(this->bFreeList);
		bp->b_forw = this->bFreeList.b_forw;
		this->bFreeList.b_forw->b_back = bp;
		this->bFreeList.b_forw = bp;
		/* 初始化自由队列 */
		bp->b_flags = myBuf::B_BUSY;
		Brelse(bp);
	}
	return;
}

myBuf* myBufferManager::GetBlk( int blkno)
{
	//cout << "BufferManager.GetBlk" << endl;
	myBuf* headbp = &this->bFreeList;
	myBuf *bp;
	//注：查看nodev队列中是否已经有该块的缓存
	//事实上本次课设中，根本不需要两个缓存队列，因为单进程情况，不可能存在B_BUSY的情况
	//但是还是为了遵循UNIX一个缓存块必定在两个缓存队列中的传统
	for (bp = headbp->b_forw; bp != headbp; bp = bp->b_forw)
	{
		if (bp->b_blkno != blkno)
			continue;
		bp->b_flags |= myBuf::B_BUSY;
	//	cout << "在缓存队列中找到对应的缓存，置为busy，GetBlk返回 blkno=" <<blkno<< endl;
		return bp;
	}

	//注：队列中没有相同的缓存块
	bp = this->bFreeList.b_forw;
	if (bp->b_flags&myBuf::B_DELWRI)
	{
	//	cout << "分配到有延迟写标记的缓存，将执行Bwrite" << endl;
		//注：有延迟写标志，在这里直接写，不做异步IO的标志
		this->Bwrite(bp);
	}
	//注：这里清空了其他所有的标志，只置为busy
	bp->b_flags = myBuf::B_BUSY;

	//注：我这里的操作是将头节点变成尾节点
	bp->b_back->b_forw = bp->b_forw;
	bp->b_forw->b_back = bp->b_back;

	bp->b_back = this->bFreeList.b_back->b_forw;
	this->bFreeList.b_back->b_forw = bp;
	bp->b_forw = &this->bFreeList;
	this->bFreeList.b_back = bp;

	bp->b_blkno = blkno;
//	cout << "成功分配到可用的缓存，getBlk将成功返回" << endl;
	return bp;

} 

void myBufferManager::Brelse(myBuf* bp)
{
							/* 注意以下操作并没有清除B_DELWRI、B_WRITE、B_READ、B_DONE标志
							* B_DELWRI表示虽然将该控制块释放到自由队列里面，但是有可能还没有些到磁盘上。
							* B_DONE则是指该缓存的内容正确地反映了存储在或应存储在磁盘上的信息
							*/
	bp->b_flags &= ~(myBuf::B_WANTED | myBuf::B_BUSY | myBuf::B_ASYNC);
	//注：这里取消了对自由缓存队列的初始化和移动操作，系统中只有一个队列


	return;
}




myBuf* myBufferManager::Bread( int blkno)
{
//	cout << "BufferManager.Bread" << endl;
	myBuf* bp;
	/* 根据设备号，字符块号申请缓存 */
	bp = this->GetBlk( blkno);
	/* 如果在设备队列中找到所需缓存，即B_DONE已设置，就不需进行I/O操作 */
	if (bp->b_flags & myBuf::B_DONE)
	{
	//	cout << "找到所需缓存，Bread将返回" << endl;
		return bp;
	}
	/* 没有找到相应缓存，构成I/O读请求块 */
	bp->b_flags |= myBuf::B_READ;
	bp->b_wcount = myBufferManager::BUFFER_SIZE;
	//我们直接将它拷贝缓存即可
	//cout << "执行读blkno=" << blkno<<"  Bread将成功返回" << endl;
	memcpy(bp->b_addr, &p[myBufferManager::BUFFER_SIZE * blkno], myBufferManager::BUFFER_SIZE);
	

	return bp;
}



void myBufferManager::Bwrite(myBuf *bp)
{
	//cout << "BufferManager.Bwrite" << endl;
	unsigned int flags;

	flags = bp->b_flags;
	bp->b_flags &= ~(myBuf::B_READ | myBuf::B_DONE | myBuf::B_ERROR | myBuf::B_DELWRI);
	bp->b_wcount = myBufferManager::BUFFER_SIZE;		/* 512字节 */

	//cout << "写到磁盘上的块号为：" << bp->b_blkno << endl;
	if ((flags & myBuf::B_ASYNC) == 0)
	{
		/* 同步写，需要等待I/O操作结束 */
		memcpy(&this->p[myBufferManager::BUFFER_SIZE*bp->b_blkno], bp->b_addr, myBufferManager::BUFFER_SIZE);
		this->Brelse(bp);
	}
	else if ((flags & myBuf::B_DELWRI) == 0)
	{
		memcpy(&this->p[myBufferManager::BUFFER_SIZE*bp->b_blkno], bp->b_addr, myBufferManager::BUFFER_SIZE);
		this->Brelse(bp);
	}
	return;
}

void myBufferManager::Bdwrite(myBuf *bp)
{
	/* 置上B_DONE允许其它进程使用该磁盘块内容 */
	bp->b_flags |= (myBuf::B_DELWRI | myBuf::B_DONE);
	this->Brelse(bp);
	return;
}

void myBufferManager::Bawrite(myBuf *bp)
{
	/* 标记为异步写 */
	bp->b_flags |= myBuf::B_ASYNC;
	this->Bwrite(bp);
	return;
}

void myBufferManager::ClrBuf(myBuf *bp)
{
//	cout << "BufferManager.ClrBuf" << endl;
	int* pInt = (int *)bp->b_addr;

	/* 将缓冲区中数据清零 */
	for (unsigned int i = 0; i < myBufferManager::BUFFER_SIZE / sizeof(int); i++)
	{
		pInt[i] = 0;
	}
//	cout << "清空buf中的数据，ClrBuf返回" << endl;
	return;
}

void myBufferManager::Bflush()
{
//	cout << "Buffermanager.Bflush" << endl;
	myBuf* bp;
	//注：单进程，这个注意不用注意
	/* 注意：这里之所以要在搜索到一个块之后重新开始搜索，
	* 因为在bwite()进入到驱动程序中时有开中断的操作，所以
	* 等到bwrite执行完成后，CPU已处于开中断状态，所以很
	* 有可能在这期间产生磁盘中断，使得bfreelist队列出现变化，
	* 如果这里继续往下搜索，而不是重新开始搜索那么很可能在
	* 操作bfreelist队列的时候出现错误。
	*/
	for (bp = this->bFreeList.b_forw; bp != &(this->bFreeList); bp = bp->b_forw)
	{
	//	cout << "该缓存块指向哪一个磁盘" << bp->b_blkno << endl;
		/* 找出自由队列中所有延迟写的块 */
		if ((bp->b_flags & myBuf::B_DELWRI) )
		{
		//	cout << "找到延迟写的块" << endl;
			//注：将它放在队列的尾部
			bp->b_back->b_forw = bp->b_forw;
			bp->b_forw->b_back = bp->b_back;

			bp->b_back = this->bFreeList.b_back->b_forw;
			this->bFreeList.b_back->b_forw = bp;
			bp->b_forw = &this->bFreeList;
			this->bFreeList.b_back = bp;

			this->Bwrite(bp);
		}
	}
	return;
}
 

myBuf* myBufferManager::InCore( int blkno)
{
	myBuf* bp;

	myBuf*headdbp = &this->bFreeList;

	for (bp = headdbp->b_forw; bp != headdbp; bp = bp->b_forw)
	{
		if (bp->b_blkno == blkno )
			return bp;
	}
	return NULL;
}


myBuf& myBufferManager::GetBFreeList()
{
	return this->bFreeList;
}

