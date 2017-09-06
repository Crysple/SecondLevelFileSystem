# SecondLevelFileSystem
A second level File System ----my "operating system" project/homework


This is the big job of my operating system course. It is a simple second level file system imitating that in linux.
It use two files(imitating the disk) to store the infomation -- the "main.dat" file and the "data.dat" file.

- "main.dat" stores infomation about users and the number of them
- "data.dat" stores i-nodes, i-node table, bitmap and blocks which directory is also stored in some blocks pertaining to a i-node

special things:
- use "adduser" command to add a user at first and use "exit" command to exit the system
- must do the "open" command every time you want to have a operation on a file.

Below is the lab report


# 操作系统实验报告

## 1.实验目的
通过一个简单多用户文件系统的设计，加深理解文件系统内部功能及内部实现
## 2.实验环境
在OSX上实验成功，理论上在windows和linux上运行源码也应该能成功。
## 3.实验内容
为**Linux**设计一个简单的二级文件系统。要求做到以下几点

 1. 可以实现下列几条命令
    - login     用户登录
    - dir       列文件目录
    - create    创建文件
    - delete    删除文件
    - open      打开文件
    - close     关闭文件
    - read      读文件
    - write     写文件
 2. 列目录时要列出文件名、物理地址、保护码和文件长度；
 3. 源文件可以进行读写保护

## 4.程序中使用的数据结构及符号说明
### a.基本程序说明
- 本文件系统采用两个分区，用两个二进制文件模拟磁盘，分别为main.dat和data.dat
-   - main.dat储存用户信息及系统的一些信息
    - data.dat储存分块的BitMap表，Inode表以及磁盘分块
    - 文件内容分布如下
        ![4.png-25.4kB][1]
    - 每个Inode最多能有10个磁盘块，没有间接磁盘块，磁盘块的大小为1KB
- 保护码的输入输出采用像Linux的三位八进制数的表示方式，比如777表示全1，但在dir中会显示成二进制
-  目录及其文件表项同样为一个文件，用inode储存于磁盘块中
-  open即是将文件inode从磁盘读取到内存，close是清除内存
### b.特殊说明
- 添加用户请在初始化时用adduser命令
- 登录请在初始化的时候用login命令
- 在登录后使用exit命令退出系统
- 每次对一个文件进行一个操作都一定必须先用open命令打开！（每个操作都要）不然会发生错误
- 写文件请在结束是加上$EOF结束符
- 登录后的命令对大小写不敏感

### c.数据结构及符号说明
```c++
#define BLOCK_SIZE 1024 //块大小
#define BLOCK_NUM 8192  //块数量
#define INODE_BIT_SIZE 200*sizeof(bool) //inode表项的大小
#define INODE_TABLE_SIZE INODE_BIT_SIZE+10*BLOCK_SIZE   //整个inode表的大小
#define BITMAP_SIZE sizeof(Bitmap)  //磁盘块位图的大小
#define MAINFILE "main.dat"
#define DATAFILE "data.dat"//该文件预留前十个块存放用户根目录inode节点

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

//磁盘![4.png-25.4kB][2]块的位图
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
```
## 5.源程序及注释
见附件📎
## 6.程序运行时的初值和运行结果
### 1.注册用户及登录，创建文件
![1.png-45kB][3]
### 2.写入文件以及读出内容
![2.png-25.3kB][4]
### 3.重新登录，查看文件长度等信息，以及删除文件
![3.png-47.4kB][5]


  [1]: http://static.zybuluo.com/jyyzzj/l5lbfjumqrxe4egc51dgx3cd/4.png
  [2]: http://static.zybuluo.com/jyyzzj/h2mwbn4i33sh9oprd3yn3sbv/4.png
  [3]: http://static.zybuluo.com/jyyzzj/exmxbhlfc8kg1veaj54v91fm/1.png
  [4]: http://static.zybuluo.com/jyyzzj/ivnsrnbcdo3wkpejbjx5wmwv/2.png
  [5]: http://static.zybuluo.com/jyyzzj/monowk4bf6bj7lgjso76uorl/3.png
