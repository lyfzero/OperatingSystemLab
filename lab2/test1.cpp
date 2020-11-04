#include<pthread.h>
#include<unistd.h>
#include<stdio.h>
#include<sys/sem.h>
#include<sys/stat.h>
#include<errno.h>
#include<stdlib.h>
#include<limits.h>


void *routineThread1(void *param);

void *routineThread2(void *param);

union semun {
    int              val;    /* Value for SETVAL */
    struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
    unsigned short  *array;  /* Array for GETALL, SETALL */
    struct seminfo  *__buf;  /* Buffer for IPC_INFO */
};
int create_Sem(int key, int size);
void destroy_Sem(int semid);
int get_Sem(int key, int size);
void set_N(int semid, int index, int val);
void P(int semid, int index);
void V(int semid, int index);


// the global semaphore
static int semId;
// the sum
static int sum = 0;
// two threads
static pthread_t thread1, thread2;
// termination flag
static int terminateFlag = 0;

int main() {
    // initialize semaphore
    key_t semkey;
    if ((semkey = ftok("/tmp", 'a')) == (key_t)(-1)) {
        perror("IPC error: ftok");
        exit(1);
    }
    semId = create_Sem(semkey, 2);
    set_N(semId, 0, 0);
    set_N(semId, 1, 1);

    // create two thread
    pthread_create(&thread1, NULL, routineThread1, NULL);
    pthread_create(&thread2, NULL, routineThread2, NULL);

    // wait for threads to finish
    pthread_join(thread2, NULL);

    // destroy semaphore
    destroy_Sem(semId);
    return 0;
}

void *routineThread1(void *param) {
    // calaulate the sum from 1 to 100
    for (int i = 1; i <= 100; ++i) {
        P(semId, 1);
        sum += i;
        V(semId, 0);
    }
    terminateFlag = 1;
    return NULL;
}

void *routineThread2(void *param) {
    while (1) {
        P(semId, 0);
        printf("temp sum = %d\n", sum);

        // try to wait for thread 1
        if (terminateFlag) {
            pthread_join(thread1, NULL);
            return NULL;
        }

        V(semId, 1);
    }
    return NULL;
}

int create_Sem(int key, int size) {
    int id;
    id = semget(key, size, IPC_CREAT | 0666);           //创建size个信号量
    if (id < 0) {                                       //判断是否创建成功
        printf("create sem %d,%d error\n", key, size);  //创建失败，打印错误
    }
    return id;
}

void destroy_Sem(int semid) {
    int res = semctl(semid, 0, IPC_RMID, 0);            //从系统中删除信号量
    if (res < 0) {                                      //判断是否删除成功
        printf("destroy sem %d error\n", semid);        //信号量删除失败，输出信息
    }
    return;
}

int get_Sem(int key, int size) {
    int id;
    id = semget(key, size, 0666);                       //获取已经创建的信号量
    if (id < 0) {
        printf("get sem %d,%d error\n", key, size);
    }
    return id;
}

void set_N(int semid, int index, int val) {
    union semun semopts;
    semopts.val = val;                                  //设定SETVAL的值为val
    semctl(semid, index, SETVAL, semopts);              //初始化信号量，信号量编号为0
    return;
}

void P(int semid, int index) {
    struct sembuf sem;                                  //信号量操作数组
    sem.sem_num = index;                                //信号量编号
    sem.sem_op = -1;                                    //信号量操作，-1为P操作
    sem.sem_flg = 0;                                    //操作标记：0或IPC_NOWAIT等
    semop(semid, &sem, 1);                              //1:表示执行命令的个数
    return;
}

void V(int semid, int index) {
    struct sembuf sem;                                  //信号量操作数组
    sem.sem_num = index;                                //信号量编号
    sem.sem_op =  1;                                    //信号量操作，1为V操作
    sem.sem_flg = 0;                                    //操作标记
    semop(semid, &sem, 1);                              //1:表示执行命令的个数
    return;
}