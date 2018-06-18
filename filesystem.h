#pragma once


#include"inode.h"
#include"file.h"
#include"openfilemanager.h"
#include"BufferManager.h"


class myOpenFileTable;
class myInodeTable;







/*
* 文件系统存储资源管理块(Super Block)的定义。
*/
class mySuperBlock
{
	/* Functions */
public:
	/* Constructors */
	mySuperBlock();
	/* Destructors */
	~mySuperBlock();

	/* Members */
public:
	int		s_isize;		/* 外存Inode区占用的盘块数 */
	int		s_fsize;		/* 盘块总数 */

	int		s_nfree;		/* 直接管理的空闲盘块数量 */
	int		s_free[100];	/* 直接管理的空闲盘块索引表 */

	int		s_ninode;		/* 直接管理的空闲外存Inode数量 */
	int		s_inode[100];	/* 直接管理的空闲外存Inode索引表 */

	//注：本次课设不需要封锁inode表和数据块索引表

	int		s_fmod;			/* 内存中super block副本被修改标志，意味着需要更新外存对应的Super Block */
	int		s_ronly;		/* 本文件系统只能读出 */
	int		s_time;			/* 最近一次更新时间 */
	int		padding[49];	/* 填充使SuperBlock块大小等于1024字节，占据2个扇区 */

};













/*
* 文件系统类(FileSystem)管理文件存储设备中
* 的各类存储资源，磁盘块、外存INode的分配、
* 释放。
*/
class myFileSystem
{
public:
	/* static consts */
	static const int SUPER_BLOCK_SECTOR_NUMBER = 0;	/* 定义SuperBlock位于磁盘上的扇区号，占据1，2两个扇区。 */

	static const int ROOTINO = 0;			/* 文件系统根目录外存Inode编号 */

	static const int INODE_NUMBER_PER_SECTOR = 8;		/* 外存INode对象长度为64字节，每个磁盘块可以存放512/64 = 8个外存Inode */
	static const int INODE_ZONE_START_SECTOR = 2;		/* 外存Inode区位于磁盘上的起始扇区号 */
	static const int INODE_ZONE_SIZE = 27 - 2/*1024 - 202*/;		/* 磁盘上外存Inode区占据的扇区数 */ //注：有修改

	static const int DATA_ZONE_START_SECTOR = 27/*1024*/;		/* 数据区的起始扇区号 */    //注：有修改
	static const int DATA_ZONE_END_SECTOR = 1000 - 1;	/* 数据区的结束扇区号 */
	static const int DATA_ZONE_SIZE = 1000 - DATA_ZONE_START_SECTOR;	/* 数据区占据的扇区数量 */

																		/* Functions */
public:
	/* Constructors */
	myFileSystem();
	/* Destructors */
	~myFileSystem();

	/*
	* @comment 初始化成员变量
	*/
	void Initialize();

	/*
	* @comment 系统初始化时读入SuperBlock
	*/
	void LoadSuperBlock();

	/*
	* @comment 根据文件存储设备的设备号dev获取
	* 该文件系统的SuperBlock
	*/
	mySuperBlock* GetFS();
	/*
	* @comment 将SuperBlock对象的内存副本更新到
	* 存储设备的SuperBlock中去
	*/
	void Update();

	/*
	* @comment  在存储设备dev上分配一个空闲
	* 外存INode，一般用于创建新的文件。
	*/
	myInode* IAlloc();						//
	/*
	* @comment  释放存储设备dev上编号为number
	* 的外存INode，一般用于删除文件。
	*/
	void IFree( int number);

	/*
	* @comment 在存储设备dev上分配空闲磁盘块
	*/
	myBuf* Alloc();
	/*
	* @comment 释放存储设备dev上编号为blkno的磁盘块
	*/
	void Free( int blkno);




private:
	/*
	* @comment 检查设备dev上编号blkno的磁盘块是否属于
	* 数据盘块区
	*/
	bool BadBlock(mySuperBlock* spb,  int blkno);




	/* Members */
public:

private:
	myBufferManager* m_BufferManager;		/* FileSystem类需要缓存管理模块(BufferManager)提供的接口 */

};






/*
* 文件管理类(FileManager)
* 封装了文件系统的各种系统调用在核心态下处理过程，
* 如对文件的Open()、Close()、Read()、Write()等等
* 封装了对文件系统访问的具体细节。
*/
class myFileManager
{
public:
	/* 目录搜索模式，用于NameI()函数 */
	enum DirectorySearchMode
	{
		OPEN = 0,		/* 以打开文件方式搜索目录 */
		CREATE = 1,		/* 以新建文件方式搜索目录 */
		DELETE = 2		/* 以删除文件方式搜索目录 */
	};

	/* Functions */
public:
	/* Constructors */
	myFileManager();
	/* Destructors */
	~myFileManager();


	/*
	* @comment 初始化对全局对象的引用
	*/
	void Initialize();

	/*
	* @comment Open()系统调用处理过程
	*/
	void Open();

	/*
	* @comment Creat()系统调用处理过程
	*/
	void Creat();

	/*
	* @comment Open()、Creat()系统调用的公共部分
	*/
	void Open1(myInode* pInode, int mode, int trf);

	/*
	* @comment Close()系统调用处理过程
	*/
	void Close();

	/*
	* @comment Seek()系统调用处理过程
	*/
	void Seek();


	/*
	* @comment FStat()获取文件信息
	*/
	void FStat();

	/*
	* @comment FStat()获取文件信息
	*/
	void Stat();

	/* FStat()和Stat()系统调用的共享例程 */
	void Stat1(myInode* pInode, unsigned long statBuf);

	/*
	* @comment Read()系统调用处理过程
	*/
	void Read();

	/*
	* @comment Write()系统调用处理过程
	*/
	void Write();

	/*
	* @comment 读写系统调用公共部分代码
	*/
	void Rdwr(enum myFile::FileFlags mode);

	/*
	* @comment 目录搜索，将路径转化为相应的Inode，
	* 返回上锁后的Inode
	*/
	myInode* NameI(char(*func)(), enum DirectorySearchMode mode);

	/*
	* @comment 获取路径中的下一个字符
	*/
	static char NextChar();

	/*
	* @comment 被Creat()系统调用使用，用于为创建新文件分配内核资源
	*/
	myInode* MakNode(unsigned int mode);

	/*
	* @comment 向父目录的目录文件写入一个目录项
	*/
	void WriteDir(myInode* pInode);

	/*
	* @comment 设置当前工作路径
	*/
	void SetCurDir(char* pathname);

	/*
	* @comment 检查对文件或目录的搜索、访问权限，作为系统调用的辅助函数
	*/
	int Access(myInode* pInode, unsigned int mode);


	/* 改变当前工作目录 */
	void ChDir();


	/* 取消文件 */
	void UnLink();

	/*创建目录*/
	void MkNod();
public:
	/* 根目录内存Inode */
	myInode* rootDirInode;

	/* 对全局对象g_FileSystem的引用，该对象负责管理文件系统存储资源 */
	myFileSystem* m_FileSystem;

	/* 对全局对象g_InodeTable的引用，该对象负责内存Inode表的管理 */
	myInodeTable* m_InodeTable;

	/* 对全局对象g_OpenFileTable的引用，该对象负责打开文件表项的管理 */
	myOpenFileTable* m_OpenFileTable;

	/*注：对全局对象g_spb的引用*/
	mySuperBlock *m_gspb;
};




//================================================filedirectory====================================================//

class myDirectoryEntry
{
	/* static members */
public:
	static const int DIRSIZ = 28;	/* 目录项中路径部分的最大字符串长度 */

									/* Functions */
public:
	/* Constructors */
	myDirectoryEntry();
	/* Destructors */
	~myDirectoryEntry();

	/* Members */
public:
	int m_ino;		/* 目录项中Inode编号部分 */
	char m_name[DIRSIZ];	/* 目录项中路径名部分 */
};






