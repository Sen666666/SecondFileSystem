#pragma once
#ifndef BUF_H
#define BUF_H

/*
* 缓存控制块buf定义
* 记录了相应缓存的使用情况等信息；
* 同时兼任I/O请求块，记录该缓存
* 相关的I/O请求和执行结果。
*/
class myBuf
{
public:
	enum BufFlag	/* b_flags中标志位 */
	{
		B_WRITE = 0x1,		/* 写操作。将缓存中的信息写到硬盘上去 */
		B_READ = 0x2,		/* 读操作。从盘读取信息到缓存中 */
		B_DONE = 0x4,		/* I/O操作结束 */
		B_ERROR = 0x8,		/* I/O因出错而终止 */
		B_BUSY = 0x10,		/* 相应缓存正在使用中 */
		B_WANTED = 0x20,	/* 有进程正在等待使用该buf管理的资源，清B_BUSY标志时，要唤醒这种进程 */
		B_ASYNC = 0x40,		/* 异步I/O，不需要等待其结束 */
		B_DELWRI = 0x80		/* 延迟写，在相应缓存要移做他用时，再将其内容写到相应块设备上 */
	};

public:
	unsigned int b_flags;	/* 缓存控制块标志位 */

	int		padding;		/* 4字节填充，使得b_forw和b_back在Buf类中与Devtab类
							* 中的字段顺序能够一致，否则强制转换会出错。 */
	/* 缓存控制块队列勾连指针 */
	//由于是单进程单设备，这里简化为一个缓存队列
	myBuf*	b_forw;
	myBuf*	b_back;

	//short	b_dev;			/* 主、次设备号，其中高8位是主设备号，低8位是次设备号 */
	int		b_wcount;		/* 需传送的字节数 */
	unsigned char* b_addr;	/* 指向该缓存控制块所管理的缓冲区的首地址 */
	int		b_blkno=-1;		/* 磁盘逻辑块号 */

	//注：不可能IO出错，不许要出错信息
};

#endif
