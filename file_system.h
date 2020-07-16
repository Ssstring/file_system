#include <semaphore.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

typedef unsigned int uint;
#define DISK "testfile"     //磁盘文件
#define BLOCKSIZE 1024      //数据块大小
#define BLOCKNUM (80*1024)  //数据块数量
#define INODENUM 1024       //inode块数量
#define INODEBLOCKNUM 1024  //每个inode最多拥有的block数
#define MaxDirNum (INODEBLOCKNUM*(BLOCKSIZE/sizeof(Dirent)))    //最大的目录项
#define DIRPERBLOCK (BLOCKSIZE/sizeof(Dirent))  //每个数据块最多存储的目录数
#define SUPERBEGIN 0                            //超级块起始地址
#define INODEBEGIN sizeof(SuperBlock)           //inode块起始地址
#define BLOCKBEGIN (INODEBEGIN+INODENUM*sizeof(Inode))  //数据块起始地址
#define Directory 0
#define File 1
#define Dirtable(inode, i) ((Dirent*)(addr+BLOCKBEGIN+BLOCKSIZE*(inode->addrs[(i)/DIRPERBLOCK])) + (i)) //取出inode对应目录项
#define InodeAddr(i) ((Inode*)(addr+INODEBEGIN)+(i))    //第i个inode所在地址
#define BLOCKADDR(i) (void*)(addr+BLOCKBEGIN+BLOCKSIZE*i)   //第i个block所在地址
// 超级块
typedef struct{
    uint inode_map[INODENUM];   //inode位图
    uint block_map[BLOCKSIZE];  //block位图
    uint inode_used;            //使用的inode数量
    uint blk_used;              //使用的blk数量
}SuperBlock;

// inode 节点
typedef struct{
    short type;     //文件类型 0：目录，1：文件
    short readcnt;  //读者数量，用于信号量机制允许多个读
    uint size;      //文件大小
    uint addrs[INODEBLOCKNUM];//使用的数据块
    uint blockNum;  //使用的数据块数量
    sem_t sem_read, sem_write;  //信号量
}Inode;
#define DIRSIZE 30  //目录名长度
// 目录
typedef struct{
    char name[DIRSIZE];
    int inode_num;
}Dirent;

int dirNum;
int inodeNum;
Inode *currInode;
SuperBlock *superBlock;
FILE* Disk;
#define CommanNum	(sizeof(cmd)/sizeof(char*))
char* cmd[] = {
    "fmt",      //格式化文件系统
    "mkdir",    //创建目录
    "rmdir",    //删除目录
    "cd",       //进入目录
    "rename",   //修改名称
    "open",     //创建文件
    "rm",       //删除文件
    "ls",       //查看文件系统目录结构
    "write",    //写文件
    "read",     //读文件
    "exit"      //退出
};
char path[40] = "rongyangou@root";
void* addr;

int init_file_system();
int open_dir(int);  // 打开目录
int format_fs(void);// 格式化文件系统
int show_dir(int);  // ls 显示
int make_file(char*, int);  //创建文件
int enter_dir(int, char*);  //进入目录
int check_name(Inode*, char*);  // 获取子文件inode
int check_type(char*);  //获取文件类型
int init_dir_inode(int);    //初始化目录
int init_file_inode(int);   //初始化文件
int get_blk();              //获取一个新的数据块
int get_inode();            //获取一个新的inode
void change_path(char*);    //改变显示路径
int rename_dir(char*, char*);
int del_file(int, char*, int);  //删除文件
int free_inode(int );       //释放inode
int rm_dir(char* );         //删除目录
int my_write(char*);        //写文件
int my_read(char*);         //读文件