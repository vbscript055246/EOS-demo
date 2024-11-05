#include <stdio.h>
#include <stdlib.h>
#include <sys/unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
int connectsock(const char *host, const char *port);
void errexit(const char *content, ...);

int main(int argc, char *argv[]){
    
    int	csock;
    int i, times;
    char buf[15];
    // char action[2][10] = {"deposit", "withdraw"};
    if(argc!=6) errexit("usage: client [IP] [port] [action] [amount] [times]\n");
    
    srand(time(NULL));
    csock = connectsock(argv[1], argv[2]);
    times = atoi(argv[5]);
    
    memset(buf, 0, sizeof(buf));
    sprintf(buf, "%s %s", argv[3], argv[4]);
    printf("strlen %d\n", strlen(buf));
    // buf[strlen(buf)] = '\0';

    while(times>0){
        printf("%03d:send <%s> to server\n", times, buf);
        write(csock, buf, 10);
        times--;
    }
    // printf("send %d times", times);
    // for(i=0;i<5;i++){
    //     sprintf(buf, "%s %d", action[rand()&1], 1 + rand() % 100);
    //     printf("send %s to server\n", buf);
    //     write(csock, buf, sizeof(buf));
    //     memset(buf, 0, sizeof(buf));
    // }
    close(csock);

}

int connectsock(const char *host, const char *port)
{
    struct sockaddr_in sin; /* an Internet endpoint address	*/
    int	s;                  /* socket descriptor and socket type	*/

    bzero((char *)&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = inet_addr(host);
    sin.sin_port = htons((u_short)atoi(port));

    /* Allocate a socket */
    s = socket(PF_INET, SOCK_STREAM, 0);
    if (s < 0) errexit("can't create socket: %s\n", strerror(errno));

    /* Connect the socket */
    if(connect(s, (struct sock_addr *)&sin, sizeof(sin)) < 0)
        errexit("Can't connect: %s\n", strerror(errno)); 

    return s;
}

void errexit(const char *content, ...){
    va_list args;
    va_start(args, content);
    vprintf(content, args);
    va_end(args);
    exit(0);
}