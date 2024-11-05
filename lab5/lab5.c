#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/unistd.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#define QLEN 50

int passivesock(const char * port, int qlen);
int printTrain(int fd);
void errexit(const char *content, ...);


void handler(int signum) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main(int argc, char *argv[]) {

    struct sockaddr_in fsin;   /* the address of a client */
    int	alen;	               /* length of client's address */
    int	msock;	               /* master server socket */
    int	ssock;	               /* slave server socket */
    if(argc!=2) errexit("usage: lab5 [port]\n");

    msock = passivesock(argv[1],QLEN);
    signal(SIGCHLD, handler);

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
                exit(printTrain(ssock));
            default:	/* parent */
                close(ssock);
				printf("Train ID:%d\n", pid);
                break;
            case -1:
                errexit("fork: %s\n", strerror(errno));
        }
    }
}

int printTrain(int fd){
    // printf("call train\n");
    dup2(fd, STDOUT_FILENO);
    execlp("sl", "sl", "-l", NULL);
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