#pragma once
#ifndef KERNEL_H
#define KERNEL_H

#include "user.h"
#include "BufferManager.h"
#include "openfilemanager.h"
#include "filesystem.h"

/*
* Kernel类用于封装所有内核相关的全局类实例对象，
* 例如PageManager, ProcessManager等。
*
* Kernel类在内存中为单体模式，保证内核中封装各内核
* 模块的对象都只有一个副本。
*/
class myKernel
{

public:
	myKernel();
	~myKernel();
	static myKernel& Instance();
	void Initialize(char*p);		/* 该函数完成初始化内核大部分数据结构的初始化 */

	myBufferManager& GetBufferManager();
	myFileSystem& GetFileSystem();
	myFileManager& GetFileManager();
	myUser& GetUser();		/* 获取当前进程的User结构 */

private:
	void InitBuffer(char*p);
	void InitFileSystem();

private:
	static myKernel instance;		/* Kernel单体类实例 */

	myBufferManager* m_BufferManager;
	myFileSystem* m_FileSystem;
	myFileManager* m_FileManager;
	myUser *m_u;

};



#endif
