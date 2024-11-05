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

#include <sys/time.h>

typedef struct {
    int guess;
    char result[8];
}data;

#define ACMODE 0666

void do_guess(int signum);
void do_update(int signum);
data *get_SHM(int SHMKEY);
void release_SHM(data *shm);
void errexit(const char *content, ...);

data *shm_data = NULL;
pid_t game_pid = -1;
int upper_bound = -1;
int lower_bound = 0;

int main(int argc, char *argv[]) {

    if(argc!=4) errexit("usage: guess <key> <upper_bound> <pid>\n");
    shm_data = get_SHM(atoi(argv[1]));
    upper_bound = atoi(argv[2]);
    game_pid = atoi(argv[3]);
    
    signal(SIGUSR1, do_update);
    signal(SIGALRM, do_guess);

    struct itimerval timer;
    timer.it_value.tv_sec = 1;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = 1;
    timer.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &timer, NULL);

    while(1) nanosleep(1);

}
void do_guess(int signum){
    shm_data->guess = (lower_bound + upper_bound) / 2;
    printf("[game] Guess: %d\n", shm_data->guess);
    kill(game_pid, SIGUSR1);
}

void do_update(int signum){
    if(!strcmp(shm_data->result, "bingo")){
        release_SHM(shm_data);
        exit(0);
    }
    if(!strcmp(shm_data->result, "bigger"))
        lower_bound = shm_data->guess;
    else 
        upper_bound = shm_data->guess;
}

data *get_SHM(int SHMKEY){
    int shmid;
    if((shmid = shmget(SHMKEY , sizeof(int), ACMODE)) < 0){
        perror("shmget");
        exit(1);
    }
    data *_data;
    if((_data = (int *)shmat(shmid, NULL, 0)) == (char *)-1){
        perror("shmat");
        exit(1);
    }
    return _data;
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