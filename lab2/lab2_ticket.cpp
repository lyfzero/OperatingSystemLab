#include<cstdio>
#include<sys/sem.h>
#include<pthread.h>
int semID;   // semaphore for ticketNum
int ticketsNum = 20;
int semNum = 1;
int semFlag;

pthread_t thread1, thread2;
void *subP1(void *i);
void *subP2(void *i);
void P(int semid, int index);
void V(int semid, int index);

int main() {
    // 创建信号灯
    semID = semget(IPC_PRIVATE, semNum, IPC_CREAT|0666);
    if(semID < 0) {
        printf("create sum failed\n");
    }
    // 信号灯赋初值1
    semctl(semID, 0, SETVAL, 1);
    // 创建线程subp1, subp2, subp3
    pthread_create(&thread1, NULL, subP1, NULL);
    pthread_create(&thread2, NULL, subP2, NULL);
    // 等待三个线程运行结束
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    // 删除信号灯
    semctl(semID, 0, IPC_RMID);
}
void *subP1(void *param) {
    for(;ticketsNum>0;) {
        P(semID, 0);
        if(ticketsNum > 0) {
            ticketsNum--;
            printf("window 1 sold one ticket, now total tickets: %d\n", ticketsNum);
        }
        V(semID, 0);
    }
    // 显示售票总数
    printf("window 1: total tickets %d\n", ticketsNum);
    return NULL;
}
void *subP2(void *param) {
    for(;ticketsNum > 0;) {
        P(semID, 0);
        if(ticketsNum > 0) {
            ticketsNum--;
            printf("window 2 sold one ticket, now total tickets: %d\n", ticketsNum);
        }
        V(semID, 0);
    }
    // 显示售票总数
    printf("window 2: total tickets %d\n", ticketsNum);
    return NULL;
}

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