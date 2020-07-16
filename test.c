#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>

typedef unsigned int uint;
#define DISK "testfile"
#define BUFF "buff"
#define BLOCKSIZE 1024
#define BLOCKNUM (80*1024)
#define INODENUM 1024
#define DIRPERBLOCK (BLOCKSIZE/sizeof(Dirent))
#define SUPERBEGIN 0
#define INODEBEGIN sizeof(SuperBlock)
#define BLOCKBEGIN (INODEBEGIN+INODENUM*sizeof(Inode))
#define Directory 0
#define File 1
// 超级块
typedef struct{
    uint inode_map[INODENUM];
    uint block_map[BLOCKSIZE];
    uint inode_used;
    uint blk_used;
}SuperBlock;

#define INODEBLOCKNUM 1024
// inode 节点
typedef struct{
    short type;
    short readcnt;
    uint size;
    uint addrs[INODEBLOCKNUM];
    uint blockNum;
    sem_t sem_read, sem_write;
}Inode;
#define DIRSIZE 30
// 目录
typedef struct{
    char name[DIRSIZE];
    short inode_num;
}Dirent;
#define MaxDirNum (INODEBLOCKNUM*(BLOCKSIZE/sizeof(Dirent)))
Dirent dir_table[MaxDirNum];
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
    "vim",      //使用vim修改文件
    "rm",       //删除文件
    "ls",       //查看文件系统目录结构
    "write",    //写文件
    "read",     //读文件
    "exit"      //退出
};
char path[40] = "rongyangou@root";
void* addr;
int init_file_system(void);
int close_file_system(void);
int format_file_system(void);
int format_fs(void);

int open_dir(int);
int close_dir(int);
int show_dir(int);
int make_file(char*, int);
int del_file(int, char*, int);
int enter_dir(int, char*);

int check_type(char*);
int check_name(Inode*, char*);

int init_dir_inode(int);
int init_file_inode(int);

int get_blk(void);
int get_inode(void);

void change_path(char*);
int rename_dir(char*, char*);

/*
    基本思路：
        1. 通过mmap在内存中分配空间，创建物理内存的映射关系，并需要了解mmap具体要怎么使用
        2. 了解文件系统，然后假设分配的内存为空白磁盘，然后设计一个文件管理系统去管理这个空白磁盘
*/
int main(){
    char comm[30], name[30];
    char new_name[DIRSIZE];
    char *arg[] = {"vim", BUFF, NULL};
    int i, exit=0, choice, status;
    int fd = open(DISK, O_RDWR);
    if(fd == -1){
        printf("open error!\n");
        return -1;
    }
    struct stat myStat;
    if(fstat(fd, &myStat)<0){
        printf("fstat error!\n");
        return -1;
    }
    // 接下来直接映射到mmap中
    addr = mmap(NULL, 104857600, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if(addr == MAP_FAILED){
        printf("mmap error!\n");
        return -1;
    }
    close(fd);
    init_file_system();
    while (1)
    {
        printf("%s# ",path);
        scanf("%s",comm);
        choice = -1;
        for(i=0;i<CommanNum;++i){
            if(strcmp(comm,cmd[i])==0){
                choice=i;
                break;
            }
        }
//         char* cmd[] = {
//     "fmt",      //格式化文件系统
//     "mkdir",    //创建目录
//     "rmdir",    //删除目录
//     "cd"
//     "rename",   //修改名称
//     "open",     //创建文件
//     "vim",      //使用vim修改文件
//     "rm",       //删除文件
//     "ls",       //查看文件系统目录结构
//     "exit"      //退出
// };
        switch (choice)
        {
        case 0:
            format_fs();
            break;
        case 1:
            // mkdir
            scanf("%s", name);
            make_file(name, Directory);
            break;
        case 2:
            // rmdir
            scanf("%s", name);
            if(check_type(name) != Directory){
                printf("rmdir: failed to remove '%s': Not a directory\n",name);
                break;
            }
            del_file(inodeNum, name, 0);
            break;
        case 3:
            // cd
            scanf("%s",name);
            if(check_type(name) != Directory){
                printf("cd: %s: Not a directory\n",name);
                break;
            }
            if(enter_dir(inodeNum, name)==0){
                change_path(name);
            }
            break;
        case 4:            
            //rename
            //根据name找出inode，修改为new_name
            //TODO: 判断new_name是否已存在
            scanf("%s %s", name, new_name);
            if(rename_dir(name, new_name) == -1){
                printf("rename: %s: no such file or directory");
            }
            break;
        case 5:
            //open
            scanf("%s", name);
            make_file(name, File);
            break;
        case 6:
            // vim
            // TODO:
            break;
        case 7:
            //rm
            scanf("%s", name);
            if(check_type(name) != File){
                printf("rm: failed to remove '%s': Not a file\n",name);
                break;
            }
            del_file(inodeNum, name, 0);
            break;
        case 8:
            //ls
            show_dir(inodeNum);
            break;
        case 9:
            //使用信号量进行同步
            scanf("%s", name);
            my_write(name);
            break;
        case 10:
            scanf("%s", name);
            my_read(name);
            break;
        case 11:
            //exit
            exit = 1;
            break;
        default:
            break;
        }
        // format_fs();
        // printf("%d %d\n", inodeNum, dirNum);
        if(exit == 1) {
            break;
        }
    }
}


int init_file_system(void) {

    superBlock = (SuperBlock*) addr;

    inodeNum = 0;

    if(!open_dir(inodeNum)){
        printf("OPEN ROOT DIR ERROR\n");
        return 0;
    }
    return 1;
}

int open_dir(int inodeNum) {
    int i;
    int pos=0;
    currInode = (Inode*)(addr + INODEBEGIN + sizeof(Inode)*inodeNum);
    dirNum = currInode->size/sizeof(Dirent);
    return 1;
}

int format_fs(void){
    memset(addr, 0, 1024*1024*100);
    // memset(superBlock->inode_map, 0, sizeof(superBlock->inode_map));
    superBlock->inode_map[0] = 1;
    superBlock->inode_used = 1;
    // memset(superBlock->block_map, 0, sizeof(superBlock->block_map));
    superBlock->block_map[0] = 1;
    superBlock->blk_used = 1;
    inodeNum = 0;
    currInode = (Inode*)(addr+INODEBEGIN);
    currInode->size = 2*sizeof(Dirent);
    currInode->blockNum = 1;
    currInode->addrs[0] = 0;
    currInode->type = Directory;
    dirNum = 2;
    strcpy(((Dirent*)(addr+BLOCKBEGIN))->name, ".");
    ((Dirent*)(addr+BLOCKBEGIN))->inode_num = 0;
    strcpy(((Dirent*)(addr+BLOCKBEGIN) + 1)->name, "..");
    ((Dirent*)(addr+BLOCKBEGIN) + 1)->inode_num = 0;
    strcpy(path,"rongyangou@root");
    return 1;
}
#define Dirtable(inode, i) ((Dirent*)(addr+BLOCKBEGIN+BLOCKSIZE*(inode->addrs[(i)/DIRPERBLOCK])) + (i))
#define InodeAddr(i) ((Inode*)(addr+INODEBEGIN)+(i))
int show_dir(int inode){
    int i, color=32;
    for (i = 0; i < currInode->size/sizeof(Dirent); i++)
    {
        if(check_type(Dirtable(currInode, i)->name) == Directory){ 
            /*目录显示绿色*/
            printf("\033[1;%dm%s\t\033[0m", color,Dirtable(currInode, i)->name);
        }
        else{
            printf("%s\t",Dirtable(currInode, i)->name);
        }
        if(!((i+1)%4)) printf("\n");
    }
    printf("\n");
    return 1;
}

int make_file(char* name, int type){
    int bnum, new_inode;
    int blockNeed = 0;
    if(dirNum>MaxDirNum){//超过了目录文件能包含的最大目录项
        printf("mkdir error: cannot create directory '%s' :Directory full\n",name);
        return -1;
    }
    if(check_name(currInode, name)!=-1){//防止重命名
        printf("mkdir error: cannnot create file '%s' :File exist\n",name);
        return -1;
    }
    if(dirNum/DIRPERBLOCK != (dirNum+1)/DIRPERBLOCK){
        blockNeed = 1;
    }
    if(blockNeed == 1){
        bnum = currInode->blockNum++;
        currInode->addrs[bnum] = get_blk();
        if(currInode->addrs[bnum] == -1){
            printf("mkdir error: cannot create file '%s' :Block used up\n", name);
            return -1;
        }
    }
    new_inode = get_inode();
    if(new_inode == -1){
        printf("mkdir: cannot create file '%s' :Inode used up\n",name);
        return -1;
    }
    if(type == Directory){
        init_dir_inode(new_inode);
    }
    else if(type == File){
        init_file_inode(new_inode);
    }
    strcpy(Dirtable(currInode, dirNum)->name, name);
    Dirtable(currInode, dirNum)->inode_num = new_inode;
    ++dirNum;
    currInode->size+=sizeof(Dirent);
    return 0;
}

int enter_dir(int inode, char* name){
    int child = check_name(currInode, name);
    if(child == -1){
        printf("cd %s error: No such file or directory\n",name);
        return -1;
    }
    InodeAddr(inodeNum)->size = dirNum*sizeof(Dirent);
    inodeNum = child;
    open_dir(child);
    return 0;
}

// get_inode_by_name
int check_name(Inode* inode, char* name){
    int i;
    for (i = 0; i < inode->size/sizeof(Dirent); i++)
    {
        if (strcmp(name, Dirtable(inode, i)->name) == 0)
        {
            return Dirtable(inode, i)->inode_num;
        }
    }
    return -1;
}

int check_type(char *name){
    int i;
    for (i = 0; i < dirNum; i++)
    {
        if (strcmp(name, Dirtable(currInode, i)->name) == 0)
        {
            return InodeAddr(Dirtable(currInode, i)->inode_num)->type;
        }
    }
    return -1;
}

int init_dir_inode(int new_inode){
    Inode *newInode = InodeAddr(new_inode);
    int block_pos = get_blk();
    newInode->blockNum = 1;
    newInode->addrs[0] = block_pos;
    newInode->size = 2*sizeof(Dirent);
    newInode->type = Directory;

    strcpy(Dirtable(newInode, 0)->name, ".");
    Dirtable(newInode, 0)->inode_num = new_inode;
    strcpy(Dirtable(newInode, 1)->name, "..");
    Dirtable(newInode, 1)->inode_num = inodeNum;
    return 0;
}

int init_file_inode(int new_inode){
    Inode *newInode = InodeAddr(new_inode);
    newInode->readcnt = 0;
    sem_init(&newInode->sem_read, 1, 1);
    sem_init(&newInode->sem_write, 1, 1);
    newInode->blockNum = 0;
    newInode->size = 0;
    newInode->type = File;
    return 0;
}

int get_blk(){
    int i;
    superBlock->blk_used++;
    for (i = 0; i < BLOCKNUM; i++)
    {
        if(!superBlock->block_map[i]){
            superBlock->block_map[i] = 1;
            return i;
        }
    }
    return -1;
}
int get_inode(){
    int i;
    superBlock->inode_used++;
    for (i = 0; i < INODENUM; i++)
    {
        if(!superBlock->inode_map[i]){
            superBlock->inode_map[i] = 1;
            return i;
        }
    }
    return -1;
}

void change_path(char* name){
    int pos;
    if(strcmp(name,".")==0){//进入本目录则路径不变
        return ;
    }
    else if(strcmp(name,"..")==0){//进入上层目录，将最后一个'/'后的内容去掉
        pos=strlen(path)-1;
        for(;pos>=0;--pos) {
            if(path[pos]=='/') {
                path[pos]='\0';
                break;
            }
        }
    }
    else {//否则在路径末尾添加子目录
        strcat(path,"/");
        strcat(path,name);
    }

    return ;
}
// 找出inode
int rename_dir(char *name, char *new_name){
    int i;
    if(strcmp(name, ".") == 0 || strcmp(name, "..") == 0){
        printf("rename error: cannot rename %s", name);
        return -1;
    }
    for (i = 2; i < dirNum; i++)
    {
        if((strcmp(Dirtable(currInode, i)->name, name)==0)){
            strcpy(Dirtable(currInode, i)->name, new_name);
        }
    }
    return 0;
}
// 递归删除
int del_file(int inode_num, char* name, int depth){
    int child_inode_num, i;
    Dirent* dir;
    Inode* child_inode;
    if(!strcmp(name, ".")||!strcmp(name, "..")){
        printf("rmdir: failed to remove '%s': Invalid argument\n",name);
        return -1;
    }
    child_inode_num = check_name(InodeAddr(inode_num), name);
    if(child_inode_num == -1) {
        printf("rmdir: failed to remove '%s': No such file or directory\n",name);
        return -1;
    }
    child_inode = InodeAddr(child_inode_num);
    if(child_inode->type == File){
        free_inode(child_inode_num);
    }
    else if(child_inode->type == Directory){
        for (i = 2; i < child_inode->size/sizeof(Dirent); i++) {
            dir = Dirtable(child_inode, i);
            del_file(child_inode_num, dir->name, depth+1);
        }
        free_inode(child_inode_num);
    }
    // 在目录中删除指定的
    if(depth == 0) {
        // 先找到被删除目录的位置
        rm_dir(name);
    }
    return 0;
}

int free_inode(int inode_num) {
    int i;
    Inode *inode = InodeAddr(inode_num);
    for (i = 0; i < inode->blockNum; i++)
    {
        free_block(inode->addrs[i]);
    }
    superBlock->inode_used--;
    superBlock->inode_map[inode_num] = 0;
    return 0;
}

int free_block(int block_num) {
    superBlock->blk_used--;
    superBlock->block_map[block_num] = 0;
    return 0;
}

int rm_dir(char* name) {
    int i;
    for (i = 0; i < dirNum; i++)
    {
        if(strcmp(Dirtable(currInode, i)->name, name) == 0) {
            break;
        }
    }
    for (i; i < dirNum-1; i++)
    {
        memcpy(Dirtable(currInode, i), Dirtable(currInode, i+1), sizeof(Dirent));
    }
    if(dirNum/DIRPERBLOCK!=(dirNum-1)/DIRPERBLOCK) {
        currInode->blockNum--;
        free_block(currInode->addrs[currInode->blockNum]);
    }
    dirNum--;
    currInode->size-=sizeof(Dirent);
    return 0;
}
#define BLOCKADDR(i) (void*)(addr+BLOCKBEGIN+BLOCKSIZE*i)
int my_write(char* name) {
    int inode_num = check_name(currInode, name);
    int bnum;
    Inode* inode = InodeAddr(inode_num);
    sem_wait(&inode->sem_write);
    char data[1025];
    scanf("%1024s", data);
    bnum = get_blk();
    inode->addrs[inode->blockNum] = bnum;
    inode->blockNum++;
    memcpy(BLOCKADDR(bnum), data, 1024);
    sleep(10);
    sem_post(&inode->sem_write);
    return 0;
}
// 首先判断文件大小
int my_read(char *name) {
    int inode_num = check_name(currInode, name);
    int bnum;
    Inode* inode = InodeAddr(inode_num);
    sem_wait(&inode->sem_read);
    inode->readcnt++;
    if(inode->readcnt == 1){
        sem_wait(&inode->sem_write);
    }
    sem_post(&inode->sem_read);
    char data[1025];
    data[1024] = 0;
    bnum = inode->addrs[0];
    memcpy(data, BLOCKADDR(bnum), 1024);
    printf("%s\n", data);
    sem_wait(&inode->sem_read);
    inode->readcnt--;
    if(inode->readcnt == 0){
        sem_post(&inode->sem_write);
    }
    sem_post(&inode->sem_read);
    return 0;
}
