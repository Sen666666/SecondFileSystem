myshell:demo1.cpp
	g++ -o myos -m32  BufferManager.cpp demo1.cpp file.cpp filesystem.cpp inode.cpp Kernel.cpp openfilemanager.cpp -std=c++11



clean:
	rm myos
