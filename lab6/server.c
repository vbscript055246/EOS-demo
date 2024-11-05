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

#include <netinet/in.h>

#include <sys/sem.h>

#define QLEN 50
#define ACMODE 0666
#define SHMKEY 9487
#define SEMKEY 9487
#define SEMCNT 1

int passivesock(const char * port, int qlen);
int create_SEM();
int get_SEM();
int lock(int sem);
int unlock(int sem);
void create_SHM();
int *get_SHM();
void release_SHM(int *shm);
int do_ATM_service(int fd);

void errexit(const char *content, ...);
void handler(int signum) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main(int argc, char *argv[]) {

    struct sockaddr_in fsin;   /* the address of a client */
    int	alen;	               /* length of client's address */
    int	msock;	               /* master server socket */
    int	ssock;	               /* slave server socket */
    int count=0;
    if(argc!=2) errexit("usage: server [port]\n");

    msock = passivesock(argv[1],QLEN);
    // signal(SIGCHLD, handler);
    
    create_SHM();
    int temp_sem = create_SEM();

    while (1) {
        alen = sizeof(fsin);
        ssock = accept(msock, (struct sockaddr *) &fsin, (socklen_t*)&alen);
        if (ssock < 0) {
            if (errno == EINTR) continue;
            errexit("accept: %s\n", strerror(errno));
        }
		pid_t pid = fork();
        switch(pid){
            case 0:		/* child */
                close(msock);
                exit(do_ATM_service(ssock));
            default:	/* parent */
                close(ssock);
                count++;
				//printf("client PID:%d\n", pid);
                break;
            case -1:
                errexit("fork: %s\n", strerror(errno));
        }
        if(count == 4){
            printf("wait\n");
            while (wait(NULL) > 0);
            printf("child done\n");
            if(semctl(temp_sem, 0, IPC_RMID, 0) < 0)
                errexit("unable to remove semaphore\n");

            int retval = shmctl(shmget(SHMKEY, sizeof(int), ACMODE), IPC_RMID, NULL);
            if(retval < 0)
                errexit("Server remove share memory failed\n");
            printf("server clean done!\n");
            return 0;
        }
    }
}

int create_SEM(){
    int s;
    s = semget(SEMKEY, 1, IPC_CREAT | IPC_EXCL | ACMODE);
    if(s<0)
        errexit("failed to create semaphore: %s\n", strerror(errno));
    printf("Semaphore %ld created\n", SEMKEY);

    if(semctl(s, 0, SETVAL, SEMCNT)<0)
        errexit("Unable to initialize semaphore: %s\n", strerror(errno));

    printf("Semaphore %ld has been initialized to 0\n", SEMKEY);
    return s;
}

int get_SEM(){
    //printf("client get SEM\n");
    int s;
    if((s = semget(SEMKEY, 1, 0))<0)
        errexit("failed to find semaphore: %s\n", strerror(errno));
    return s;
}

int lock(int sem)
{   
    
    struct sembuf sop ; /* the operation parameters */
    sop.sem_num = 0 ;   /* access the 1st (and only) sem in the array */
    sop.sem_op = -1;    /* wait .. */
    sop.sem_flg = 0 ;   /* no special options needed */
    //printf("client lock\n");
    if(semop(sem, &sop, 1) < 0)
        errexit("lock():semop failed : %s \n",strerror(errno));
    else return 0;
}

int unlock(int sem)
{   
    
    struct sembuf sop;  /* the operation parameters */
    sop.sem_num = 0;    /* the 1st (and only) sem in the array */
    sop.sem_op = 1;     /* signal */
    sop.sem_flg = 0;    /* no special options needed */
    //printf("client unlock\n");
    if(semop(sem, &sop, 1) < 0){
        errexit("unlock():semop failed : %s \n",strerror(errno));
    } 
    else return 0;
}

void create_SHM(){
    int shmid;
    int *deposit_var;

    if((shmid = shmget(SHMKEY , sizeof(int), IPC_CREAT | ACMODE) ) < 0 ) {
        perror("shmget");
        exit(1);
    }

    if((deposit_var = shmat(shmid, NULL, 0)) == ( char *)-1){
        perror("shmat");
        exit(1);
    }

    printf("Server create and attach the share memory. \n");
    *deposit_var = 0;
    printf("Server write 0 to the share memory. \n");
}

int *get_SHM(){
    int shmid;
    int *deposit;
    if((shmid = shmget(SHMKEY , sizeof(int), ACMODE)) < 0){
        perror("shmget");
        exit(1);
    }

    if((deposit = (int *)shmat(shmid, NULL, 0)) == (char *)-1){
        perror("shmat");
        exit(1);
    }
    return deposit;
}

void release_SHM(int *shm){
    shmdt(shm);
}

int do_ATM_service(int fd){
    
    char buf[15], action[20];
    int retval, amount, i=0;
    int sem = get_SEM();
    int *deposit = get_SHM();

    memset(buf, 0, 15);
    while(retval = read(fd, buf, 10)){
        if(retval<0){
            // unlock(sem);
            continue; 
        }
        i++;
        //errexit("read failed: %s\n", strerror(errno));
        
        // printf("recv <%s> <%d>\n", buf, strlen(buf));
        sscanf(strdup(buf), "%s %d", action, &amount);
        
        lock(sem);
        // printf("read deposit: %d\n", *deposit);
        if(strcmp(action, "deposit") == 0)
            (*deposit) += amount;
        else if(strcmp(action, "withdraw") == 0)
            (*deposit) -= amount;
        else{
            printf("action <%s> amount <%d>\n", action, amount);
            printf("\n\nAction error\n\n");
        }
        unlock(sem);
        // printf("write deposit: %d\n", *deposit);

        sprintf(buf, "After %s: %d", action, *deposit);
        printf("%s\n", buf);
        memset(buf, 0, 15);
    }
    // sleep(1);
    
    //printf("clinet leave\n");
    release_SHM(deposit);
    //printf("SHM DT\n");
    // printf("total :%d\n", i);
    return 0;
}

int passivesock(const char * port, int qlen)
{
    struct sockaddr_in sin; /* an Internet endpoint address	*/
    int	s;                  /* socket descriptor and socket type	*/

    bzero((char *)&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons((u_short)atoi(port));

    /* Allocate a socket */
    s = socket(PF_INET, SOCK_STREAM, 0);
    if (s < 0) errexit("can't create socket: %s\n", strerror(errno));

    /* Bind the socket */
    if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
        errexit("can't bind to port: %s\n", strerror(errno));

    if (listen(s, qlen) < 0) errexit("can't listen on port: %s\n", strerror(errno));
    return s;
}

void errexit(const char *content, ...){
    va_list args;
    va_start(args, content);
    vprintf(content, args);
    va_end(args);
    exit(0);
}