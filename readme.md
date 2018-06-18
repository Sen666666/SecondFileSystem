程序设计环境：
运行平台：@RHEL74-X64
	编 译 器：线程模型：posix
			  gcc 版本 4.8.5 20150623 (Red Hat 4.8.5-16) (GCC)

说明：因为是64系统在该版本的gcc编译下，指针将会是8位长度，这和UNIX V6++的设计不同会带来一系列问题，如此时指针类型和int型之间的转化将会截断，C++会认为这是个不安全的操作而报错，所以需要在编译选项中加上 –m32（详细可见Makefile文件） 可能会需要手动安装相应的32位库。特此说明。

项目说明：
	二级文件系统用一个宿主机上的大文件模拟磁盘，其管理的所有文件内容及文件系统的元数据存放在宿主机文件的相应位置。该二级文件系统的设计与UNIX V6++的设计完全相同。磁盘文件c.img中按照类ext2中一个分区的格式分布，取消了前200个盘块的引导块和内核信息，直接从SUperBlock开始规划，DiskInode区，数据盘块区。磁盘的块大小设计沿用了UNIX V6++的512字节。
	与UNIX V6++相比不同的是取消了多进程的操作，即取消了锁结构和解决cpu抢占问题的系列操作；取消了设备驱动，整个磁盘文件通过mmap映射到内存，直接用memcpy进行“磁盘”读写；取消了特殊文件的处理，UNIX世界中一切皆文件的设计使得原本的文件系统需要处理很多特殊设备，在本次二级文件系统中只存在数据文件；取消了多设备的各项操作，这意味着本次文件系统中只存在一个缓存队列。
	
文件说明：
	Buf.h中定义了myBuf类
	BufferManager.h中定义了myBufferManager类
	File.h中定义了myFile，myOpenFiles，IOParemeter三个类
	Filesystem.h中定义了mySuperBlock，myFileSystem，myDirectoryEntry三个类
	Inode.h中定义了myInode，DiskInode两个类
	Kernel.h中主要定义了myKernel类
	Openfilemanager.h中主要定义了myOpenFIleTable，myInodeTable两个类
	User.h中主要定义了myUser类

	对应的cpp文件是对应头文件的类的实现，而demo.cpp中实现了磁盘的格式化，系统的初始化，和文件操作的API。
	最后make出的可执行文件名为myos
	
执行说明：
	直接将所有文件放入一个文件夹下，执行 make 指令make出可执行文件myos，run ./myos 即可进入文件系统，进入用户界面之后会有更加详细的使用说明，根据提示一步步输入即可，
	实现的主要功能：
a   fopen(char *name, int mode)                     
b   fclose(int fd)                                        
c   fread(int fd, int length)                         
d   fwrite(int fd, char *buffer, int length)            
e   flseek(int fd, int position, int ptrname)        
f   fcreat(char *name, int mode)                      
g   fdelete(char *name)                               
h   ls()                                                   
i   mkdir(char* dirname)                               
j   cd(char* dirname)                                  
k   backDir()--返回上级目录                             
q  退出文件系统          
