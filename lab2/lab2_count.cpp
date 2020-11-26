#include<cstdio>
#include<sys/sem.h>
#include<pthread.h>


int semID;   // semaphore (0 for a)
int a;  // sum
int semNum = 3;
int semFlag;
int isTerminated = 0;

pthread_t thread1, thread2, thread3;
void *write(void *param);
void *read1(void *param);
void *read2(void *param);
void P(int semid, int index);
void V(int semid, int index);

int main() {
    // 创建信号灯
    semID = semget(IPC_PRIVATE, semNum, IPC_CREAT|0666);
    if(semID < 0) {
        printf("create sum failed\n");
    }
    // 信号灯赋初值1
    semctl(semID, 0, SETVAL, 1);  // write
    semctl(semID, 1, SETVAL, 0);  // read1
    semctl(semID, 2, SETVAL, 0);  // read2
    // 创建线程
    pthread_create(&thread1, NULL, write, NULL);
    pthread_create(&thread2, NULL, read1, NULL);
    pthread_create(&thread3, NULL, read2, NULL);
    // 等待三个线程运行结束
    pthread_join(thread2, NULL);
    pthread_join(thread3, NULL);
    // 删除信号灯
    semctl(semID, 0, IPC_RMID);
    semctl(semID, 1, IPC_RMID);
    semctl(semID, 2, IPC_RMID);
}
void *write(void *param) {
    for(int i=1; i<=100; i++) {
        P(semID, 0);
        a += i;
        if(a%2==0)
            V(semID, 1);
        else V(semID, 2);
    }
    return NULL;
}
void *read1(void *param) {
    while(true) {
        P(semID, 1);
        printf("read 1: sum = %d\n", a);
        V(semID, 0);
        if(a>=5050)
            break;
    }
    return NULL;
}
void *read2(void *param) {
    while(true) {  
        P(semID, 2);
        printf("read 2: sum = %d\n", a);
        V(semID, 0);
        if(a>=4851)
           break;
    }
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