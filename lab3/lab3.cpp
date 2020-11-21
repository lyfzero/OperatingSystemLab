#include <sys/sem.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include<fcntl.h>
#include <sys/wait.h>

#define BLOCKSIZE 256             // 每个缓冲区大小
#define LENGTH 20                 // 环形缓冲区长度
#define MEM_KEY 33333
#define MTX_KEY 3333

struct Block {                 // 一个缓冲区
    char data[BLOCKSIZE];
};

static int head_p_shmid, tail_p_shmid, block_shmid[LENGTH], read_n_shmid;
static int left, used;
static int wrt_p, read_p, status;

static int input_fd, target_fd;    // 读入和目标文件标识符
int *head_p, *tail_p, *read_n;    // 共享内存指针


void P(int semid, int index) {
    struct sembuf sem;
    sem.sem_num = index;
    sem.sem_op = -1;
    sem.sem_flg = 0;
    semop(semid, &sem, 1);
}
void V(int semid, int index) {
    struct sembuf sem;
    sem.sem_num = index;
    sem.sem_op = 1;
    sem.sem_flg = 0;
    semop(semid, &sem, 1);
}

static void sigint_handler_parent(int sig) {
    printf("main detect SIGINT\n");
    fflush(stdout);
    kill(wrt_p, SIGUSR1);
    kill(read_p, SIGUSR2);
    waitpid(wrt_p, &status, 0);
    waitpid(read_p, &status, 0);
    semctl(left, 0, IPC_RMID);
    semctl(used, 0, IPC_RMID);
    for (int i = 0; i < LENGTH; ++i) {
        shmctl(block_shmid[i], IPC_RMID, 0);
    }
    shmctl(head_p_shmid, IPC_RMID, 0);
    shmctl(tail_p_shmid, IPC_RMID, 0);
    shmctl(read_n_shmid, IPC_RMID, 0);
    exit(-1);
}
static void sig_handler_writebuf(int sig) {
    printf("writebuf detect kill SIG\n");
    fflush(stdout);
    close(input_fd);
    exit(-1);
}
static void sig_handler_readbuf(int sig) {
    printf("readbuf detect kill SIG\n");
    fflush(stdout);
    close(target_fd);
    exit(-1);
}

void writebuf(char * inputfile) {
    if ((input_fd = open(inputfile, O_RDONLY)) == -1) {   // 打开input文件
        fprintf(stderr, "open %s error!\n", inputfile);
        fflush(stderr);
        exit(-1);     
    }
    
    signal(SIGINT, SIG_IGN);
    signal(SIGUSR1, sig_handler_writebuf);
    struct Block *block;             // 缓冲区

    while (1) {
        P(left, 0);                 // 剩下的
        block = (struct Block*)shmat(block_shmid[*tail_p], NULL, SHM_W);
        *read_n = read(input_fd, block->data, BLOCKSIZE);
        printf("writebuf: %d, num: %d\n", *tail_p, *read_n);
        fflush(stdout);
        ++*tail_p;
        if (*tail_p == LENGTH) 
            *tail_p = 0;
        V(used, 0);
        if (*read_n != BLOCKSIZE) {
            printf("writebuf EOF\n");
            fflush(stdout);
            close(input_fd);
            exit(0);
        }
    }
}

void readbuf(char *targetfile) {
    if ((target_fd = open(targetfile, O_WRONLY)) == -1) {   // 打开target文件
        fprintf(stderr, "open %s error!\n", targetfile);
        fflush(stderr);
        exit(-1);     
    }

    signal(SIGINT, SIG_IGN);
    signal(SIGUSR2, sig_handler_readbuf);

    struct Block *block;
    while (1) {
        P(used, 0);
        if (*read_n != BLOCKSIZE && (*head_p == *tail_p-1 || *head_p == *tail_p-1 + LENGTH)) {  // EOF
            if (*read_n) { // EOF之前还有数据
                block = (struct Block*)shmat(block_shmid[*head_p], NULL, SHM_R);
                write(target_fd, block->data, *read_n);
                V(left, 0);
                printf("readbuf: %d\n", *head_p);
            }
            printf("readbuf EOF\n");
            fflush(stdout);
            close(target_fd);
            exit(0);
        }
        block = (struct Block*)shmat(block_shmid[*head_p], NULL, SHM_R);
        write(target_fd, block->data, BLOCKSIZE);
        printf("readbuf: %d\n", *head_p);
        fflush(stdout);
        ++*head_p;
        if (*head_p == LENGTH) *head_p = 0;
        V(left, 0);
    } 
}

int main(int argc, char *argv[]) {
    // 创建信号灯
    if ((left = semget(MTX_KEY, 1, IPC_CREAT|0600)) == -1) {
        fprintf(stderr, "left create error!\n");
        fflush(stderr);
        exit(-1);
    }
    if ((used = semget(MTX_KEY+1, 1, IPC_CREAT|0600)) == -1) {
        fprintf(stderr, "used create error!\n");
        fflush(stderr);
        exit(-1);
    }
    // 信号灯赋初值
    semctl(left, 0, SETVAL, LENGTH);
    semctl(used, 0, SETVAL, 0);
    // 创建共享内存组  环形缓冲
    if ((head_p_shmid = shmget(MEM_KEY+LENGTH, sizeof(int),  IPC_CREAT | 0600)) == -1) {
        fprintf(stderr, "main head_p_shmid semget error!\n");
        fflush(stderr);
        exit(-1);       
    }
    if ((tail_p_shmid = shmget(MEM_KEY+LENGTH+1, sizeof(int),  IPC_CREAT | 0600)) == -1) {
        fprintf(stderr, "main tail_p_shmid semget error!\n");
        fflush(stderr);
        exit(-1);    
    }

    if ((read_n_shmid = shmget(MEM_KEY+LENGTH+2, sizeof(int),  IPC_CREAT | 0600)) == -1) {
        fprintf(stderr, "main read_n_shmid semget error!\n");
        fflush(stderr);
        exit(-1);           
    }

    for (int i = 0; i < LENGTH; ++i) {
        if ((block_shmid[i] = shmget(MEM_KEY+i, sizeof(struct Block),  IPC_CREAT | 0600)) == -1) {
            fprintf(stderr,"main block_shmid semget error!\n");
            fflush(stderr);
            exit(-1);
        }
    }
    // if ((int)(head_p = shmat(head_p_shmid, NULL, SHM_R | SHM_W)) == -1) {   // 将要读的位置
    //     fprintf(stderr, "main head_p shmat error!\n");
    //     fflush(stderr);
    //     exit(-1);         
    // }
    // if ((int)(tail_p = shmat(tail_p_shmid, NULL, SHM_R | SHM_W)) == -1) {   // 将要写的位置
    //     fprintf(stderr, "main tail_p shmat error!\n");
    //     fflush(stderr);
    //     exit(-1);         
    // }
    // if ((int)(read_n = shmat(read_n_shmid, NULL, SHM_R | SHM_W)) == -1) {   // writebuf最近一次读到的字节数
    //     fprintf(stderr, "main read_n shmat error!\n");
    //     fflush(stderr);
    //     exit(-1);         
    // }
    head_p = (int*)shmat(head_p_shmid, NULL, SHM_R | SHM_W);
    tail_p = (int*)shmat(tail_p_shmid, NULL, SHM_R | SHM_W);
    read_n = (int*)shmat(read_n_shmid, NULL, SHM_R | SHM_W);
    *head_p = 0;
    *tail_p = 0;
    *read_n = 0;

    // 创建两个进程readbuf、writebuf
    if ((wrt_p = fork()) == 0) {
        writebuf(argv[1]);  // 从input文件读并写入buf
    } else if ((read_p = fork()) == 0) {
        readbuf(argv[2]);  // 从buf读并写入output文件
    }

    signal(SIGINT, sigint_handler_parent);

    // 等待两个进程运行结束
    waitpid(wrt_p, &status, 0);
    waitpid(read_p, &status, 0);

    // 删除信号灯
    semctl(left, 0, IPC_RMID);
    semctl(used, 0, IPC_RMID);

    // 删除共享内存组
    for (int i = 0; i < LENGTH; ++i) {
        shmctl(block_shmid[i], IPC_RMID, 0);
    }
    shmctl(head_p_shmid, IPC_RMID, 0);
    shmctl(tail_p_shmid, IPC_RMID, 0);
    shmctl(read_n_shmid, IPC_RMID, 0);
    exit(0);
}