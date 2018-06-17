#pragma once

#include<iostream>
#include"file.h"
#include"filesystem.h"

/*
* @comment 该类与Unixv6中 struct user结构对应，因此只改变
* 类名，不修改成员结构名字，关于数据类型的对应关系如下:
*/
class myUser
{
public:
	//注：不需要eax寄存器设置
								/* u_error's Error Code */
								/* 1~32 来自linux 的内核代码中的/usr/include/asm/errno.h, 其余for V6++ */
	enum ErrorCode
	{
		my_NOERROR = 0,	/* No error */
		my_EPERM = 1,	/* Operation not permitted */
		my_ENOENT = 2,	/* No such file or directory */
		my_ESRCH = 3,	/* No such process */
		my_EINTR = 4,	/* Interrupted system call */
		my_EIO = 5,	/* I/O error */
		my_ENXIO = 6,	/* No such device or address */
		my_E2BIG = 7,	/* Arg list too long */
		my_ENOEXEC = 8,	/* Exec format error */
		my_EBADF = 9,	/* Bad file number */
		my_ECHILD = 10,	/* No child processes */
		my_EAGAIN = 11,	/* Try again */
		my_ENOMEM = 12,	/* Out of memory */
		my_EACCES = 13,	/* Permission denied */
		my_EFAULT = 14,	/* Bad address */
		my_ENOTBLK = 15,	/* Block device required */
		my_EBUSY = 16,	/* Device or resource busy */
		my_EEXIST = 17,	/* File exists */
		my_EXDEV = 18,	/* Cross-device link */
		my_ENODEV = 19,	/* No such device */
		my_ENOTDIR = 20,	/* Not a directory */
		my_EISDIR = 21,	/* Is a directory */
		my_EINVAL = 22,	/* Invalid argument */
		my_ENFILE = 23,	/* File table overflow */
		my_EMFILE = 24,	/* Too many open files */
		my_ENOTTY = 25,	/* Not a typewriter(terminal) */
		my_ETXTBSY = 26,	/* Text file busy */
		my_EFBIG = 27,	/* File too large */
		my_ENOSPC = 28,	/* No space left on device */
		my_ESPIPE = 29,	/* Illegal seek */
		my_EROFS = 30,	/* Read-only file system */
		my_EMLINK = 31,	/* Too many links */
		my_EPIPE = 32,	/* Broken pipe */
		my_ENOSYS = 100
		//EFAULT	= 106
	};

	//注：不需要信号



public:
	//注：没有pcb


	/* 系统调用相关成员 */

	//注：只保留与文件系统相关的参数
	//[1]是什么 本次当前操作的文件的偏移量---好像需要通过seek'操作改变File结构中的偏移量
	//[0]是本次操作的文件句柄 fd   好像不止这样 chdir函数中又将它做pathname'？
	//[2]是什么用//。？？
	int u_arg[5];				/* 存放当前系统调用参数 */
	char* u_dirp;				/* 系统调用参数(一般用于Pathname)的指针 */

						/* 文件系统相关成员 */
	myInode* u_cdir;		/* 指向当前目录的Inode指针 */
	myInode* u_pdir;		/* 指向父目录的Inode指针 */

	myDirectoryEntry u_dent;					/* 当前目录的目录项 */
	char u_dbuf[myDirectoryEntry::DIRSIZ];	/* 当前路径分量 */
	char u_curdir[128];						/* 当前工作目录完整路径 */

	ErrorCode u_error;			/* 存放错误码 */
	
	int u_ar0;//注：本次课设中简化为int形

						/* 文件系统相关成员 */
	myOpenFiles u_ofiles;		/* 进程打开文件描述符表对象 */

							/* 文件I/O操作 */
	IOParameter u_IOParam;	/* 记录当前读、写文件的偏移量，用户目标区域和剩余字节数参数 */


};  

