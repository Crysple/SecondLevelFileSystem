//
//  FileSys.hpp
//  SecondLevelFileSystem
//
//  Created by Eser on 31/08/2017.
//  Copyright © 2017 Crysple. All rights reserved.
//

#ifndef FileSys_h
#define FileSys_h

#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
using namespace std;
#define BLOCK_SIZE 1024 //块大小
#define BLOCK_NUM 8192  //块数量
#define INODE_BIT_SIZE 200*sizeof(bool) //inode表项的大小
#define INODE_TABLE_SIZE INODE_BIT_SIZE+10*BLOCK_SIZE   //整个inode表的大小
#define BITMAP_SIZE sizeof(Bitmap)  //磁盘块位图的大小
#define MAINFILE "main.dat"
#define DATAFILE "data.dat"//该文件预留前十个块存放用户根目录inode节点
#define FILENAME 0
#define ADDRESS 1
#define PROTECTCODE 2
#define LENGTH 3
//Inode 节点
class Inode{
public:
    Inode(size_t BN):blockNum(BN){}
    Inode(){}
    //存储iNode的磁盘块的地址
    size_t blockAddress[10];
    //该iNode磁盘块的个数
    size_t blockNum;
};

//块的位图
class Bitmap{
public:
    void init();
    //得到下一个可用的块的地址
    size_t getNext();
    //释放该块
    void release(size_t add);
private:
    //块的位图，false为可用，true为已占用
    bool bitmap[BLOCK_NUM];
    //将位图写到磁盘
    void write();
};

//Inode 表
class InodeTable{
public:
    InodeTable();
    //添加一个节点，返回其地址
    size_t add(Inode * i);
    //使用inode地址获取一个inode指针，废弃需delete
    Inode* getInode(size_t add);
    //输入地址，释放该inode的所有块以及该inode本身
    void release(size_t add,Bitmap* bitmap);
private:
    //inode表，false表示空闲，true为占用
    bool entry[200];
    //得到一个可以用的inode索引
    size_t getAvailable();
    //将表写入到磁盘
    void write();
};

//文件表项，实际储存于inode中
class FileEntry{
public:
    //文件名
    char fileName[30];
    //文件地址
    size_t address;
    //保护位
    int protectCode;
    //文件长度
    size_t length;
};

//用户
class User{
public:
    //用户名
    char username[30];
    //密码
    char password[30];
    //根目录的inode地址
    size_t inodeAddress;
    void setUsername(string name);
};

//文件类，集成了处理inode的各种方法
class File{
public:
    File(Inode* _i):inode(_i){readBitmap();}
    File(){readBitmap();}
    ~File(){}
    //绑定inode到当前类
    void setInode(Inode* i){inode=i;}
    //得到该inode
    Inode* getInode(){return inode;}
    //得到当前位图
    Bitmap* getBitmap(){return &bitmap;}
    //如inode为目录，解析内容成表项数组形式
    vector<FileEntry> getDir();
    //写inode为目录
    void writeDir(const vector<FileEntry>& dir, size_t inode_address);
    //读取inode所有内容
    string readAll();
    //写inode的内容
    size_t write(const string& towrite,size_t);
    //将inode本身写入磁盘
    void writeSelf(size_t address);
    //将字符串分成块的形式
    vector<string> split(const string& now);
    //从文件读取位图
    void readBitmap();
private:
    Inode* inode;
    Bitmap bitmap;
};

class FileSys{
public:
    FileSys();
    void run();
private:
    string prefix;
    User user;
    File file;
    vector<FileEntry> fileEntrys;
    FileEntry* nowFileEntry;
    InodeTable inodeTable;
    Inode rootDir;
    string fileName;
    void pln(const string& s);
    void p(const string& s);
    void printDir();
    void enterFileName();
    void createFile();
    void deleteFile();
    void openFile();
    void closeFile();
    void readFile();
    void writeFile();
    void boot();
    string pcToBinary(int n);
    bool login();
    void adduser();
    void init();
};

#endif /* FileSys_hpp */
