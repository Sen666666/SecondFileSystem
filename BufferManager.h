#pragma once
#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H

#include "Buf.h"
//#include "DeviceManager.h"


//注：本次课设中buffermanger只需处理与mmap申请的内存的交互，mmap申请的内存就相当于磁盘
//同时1.由于只有一个文件系统，不再设置dev编号做设备区分
//2.不需要再负责进程图像的交换

//考虑了很久，考虑到时间关系就不做设备驱动了，
//buffermanager将被重写，直接控制自己的缓存，不再向下调用设备驱动
class myBufferManager
{
public:
	/* static const member */
	static const int NBUF = 15;			/* 缓存控制块、缓冲区的数量 */
	static const int BUFFER_SIZE = 512;	/* 缓冲区大小。 以字节为单位 */

public:
	myBufferManager();
	~myBufferManager();

	void Initialize(char *start);					/* 缓存控制块队列的初始化。将缓存控制块中b_addr指向相应缓冲区首地址。*/

	//注：真实的系统中有dev编号，本次课设中没有

	myBuf* GetBlk(int blkno);				/* 申请一块缓存，用于读写设备dev上的字符块blkno。*/
	void Brelse(myBuf* bp);				/* 释放缓存控制块buf */

	myBuf* Bread(int blkno);	/* 读一个磁盘块。dev为主、次设备号，blkno为目标磁盘块逻辑块号。 */
	myBuf* Breada(short adev, int blkno, int rablkno);	/* 读一个磁盘块，带有预读方式。
														* adev为主、次设备号。blkno为目标磁盘块逻辑块号，同步方式读blkno。
														* rablkno为预读磁盘块逻辑块号，异步方式读rablkno。 */

	void Bwrite(myBuf* bp);				/* 写一个磁盘块 */
	void Bdwrite(myBuf* bp);				/* 延迟写磁盘块 */
	void Bawrite(myBuf* bp);				/* 异步写磁盘块 */

	void ClrBuf(myBuf* bp);				/* 清空缓冲区内容 */
	void Bflush();						/* 将dev指定设备队列中延迟写的缓存全部输出到磁盘 */

	myBuf& GetBFreeList();				/* 获取自由缓存队列控制块Buf对象引用 */

private:
	myBuf* InCore(int blkno);	/* 检查指定字符块是否已在缓存中 */

private:
	myBuf bFreeList;						/* 自由缓存队列控制块 */
	//一个buf对应一个存储块
	myBuf m_Buf[NBUF];					/* 缓存控制块数组 */
	unsigned char Buffer[NBUF][BUFFER_SIZE];	/* 缓冲区数组 */

	//这是整个文件系统用mmap映射到内存后的起始地址
	char *p;
};

#endif
