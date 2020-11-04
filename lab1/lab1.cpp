#include<unistd.h>
#include<signal.h>
#include<cstdio>
#include<cstring>
#include<sys/wait.h>
#include<cstdlib>

void Child1(int pipefd[2]);
void Child2(int pipefd[2]);
void Parent(int pipefd[2]);
void error(char *error_msg);
void handleChild1(int sig);
void handleChild2(int sig);
void handleParent(int sig);
#define MAX_BUFFER_SIZE 1024

pid_t child1Pid, child2Pid;
unsigned int Child1Cnt, Child2Cnt;

int main() {
    signal(SIGINT, handleParent);

    // create pipe
    int pipefd[2];
    pipe(pipefd);

    // fork child 1
    child1Pid = fork();
    if(child1Pid < 0) {
        printf("fork child1 failed");
        return -1;
    } else if( child1Pid == 0) {
        /* child 1 */
        signal(SIGINT, SIG_IGN);
        signal(SIGUSR1, handleChild1);
        Child1(pipefd);
    } else {
        // fork child 2
        child2Pid = fork();
        if(child2Pid < 0) {
            printf("fork child2 failed");
            return -1;
        } else if(child2Pid == 0) {
            /* child 2*/
            signal(SIGINT, SIG_IGN);
            signal(SIGUSR2, handleChild2);
            Child2(pipefd);
        } else {
            /* parent */
            Parent(pipefd);
        }
    }
}

void Child1(int pipefd[2]) {
    close(pipefd[1]); // close write pipe

    char buffer[MAX_BUFFER_SIZE];
    ssize_t size;
    while(true) {
        memset(buffer, 0, MAX_BUFFER_SIZE);
        size = read(pipefd[0], buffer, MAX_BUFFER_SIZE);
        if(size < 0)
            printf("buffer is empty");
        buffer[size] = 0;
        printf("child 1: %s\n", buffer);
        Child1Cnt++;
    } 
}
void Child2(int pipefd[2]) {
    close(pipefd[1]); // close write pipe
    char buffer[MAX_BUFFER_SIZE];
    ssize_t size;
    while(true) {
        memset(buffer, 0, MAX_BUFFER_SIZE);
        size = read(pipefd[0], buffer, MAX_BUFFER_SIZE);
        if(size < 0)
            printf("buffer is empty");
        buffer[size] = 0;
        printf("child 2: %s\n", buffer);
        Child2Cnt++;
    } 
}
void Parent(int pipefd[2]) {
    close(pipefd[0]); // close read pipe
    char buffer[MAX_BUFFER_SIZE];
    unsigned sendCnt = 1;
    while(true) {
        sleep(1);
        sprintf(buffer, "I send you %u times", sendCnt);
        write(pipefd[1], buffer, MAX_BUFFER_SIZE);
        sendCnt++;
    }
}


void handleChild1(int sig) {
    printf("Child process 1 is killed by parent, run %u times!\n", Child1Cnt);
    exit(0);
}
void handleChild2(int sig) {
    printf("Child process 2 is killed by parent, run %u times!\n", Child2Cnt);
    exit(0);
}
void handleParent(int sig) {
    kill(child1Pid, SIGUSR1);
    kill(child2Pid, SIGUSR2);
    int status;
    waitpid(child1Pid, &status, 0);
    waitpid(child2Pid, &status, 0);
    printf("Parent process is killed!\n");
    exit(0);
}