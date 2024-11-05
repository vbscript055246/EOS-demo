#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/unistd.h>

#include <sys/errno.h>
#include <signal.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include <sys/wait.h>
#include <sys/types.h>

typedef struct {
    int guess;
    char result[8];
}data;

#define ACMODE 0666

void do_guess(int signo , siginfo_t *info , void *context);
void cleanup(int signum);
data *create_SHM();
void release_SHM(data *shm);
void errexit(const char *content, ...);

data *shm_data = NULL;
int answer = -1;
int SHMKEY=-1;

int main(int argc, char *argv[]) {
    if(argc!=3) errexit("usage: game <key> <guess>\n");
    SHMKEY = atoi(argv[1]);
    shm_data = create_SHM();
    answer = atoi(argv[2]);

    struct sigaction handler;
    memset(&handler, 0, sizeof(handler));
    handler.sa_flags = SA_SIGINFO;
    handler.sa_sigaction = do_guess;
    sigaction(SIGUSR1, &handler, NULL);

    signal(SIGINT, cleanup);

    printf("Game PID: %d\n", getpid());

    while(1) nanosleep(1);

}

void do_guess(int signo , siginfo_t *info , void *context){
    // printf("Process(%d) sent SIGUSR1 \n" , info->si_pid);
    
    if(shm_data->guess == answer){
        strcpy(shm_data->result, "bingo");
    }
    else if(shm_data->guess > answer){
        strcpy(shm_data->result, "smaller");
    }
    else{
        strcpy(shm_data->result, "bigger");
    }

    printf("[game] Guess: %d, %s\n", shm_data->guess, shm_data->result);
    kill(info->si_pid, SIGUSR1);
}

void cleanup(int signum) {
    release_SHM(shm_data);
    int retval = shmctl(shmget(SHMKEY, sizeof(data), ACMODE), IPC_RMID, NULL);
    if(retval < 0)
        errexit("remove share memory failed\n");
    printf("SHM clean done!\n");
    exit(0);
}

data *create_SHM(){
    int shmid;
    data *var_data;

    if((shmid = shmget(SHMKEY , sizeof(data), IPC_CREAT | ACMODE) ) < 0 ) {
        perror("shmget");
        exit(1);
    }

    if((var_data = shmat(shmid, NULL, 0)) == (char *)-1){
        perror("shmat");
        exit(1);
    }
    printf("create and attach the share memory. \n");
    return var_data;
}

void release_SHM(data *shm){
    shmdt(shm);
}

void errexit(const char *content, ...){
    va_list args;
    va_start(args, content);
    vprintf(content, args);
    va_end(args);
    exit(0);
}
