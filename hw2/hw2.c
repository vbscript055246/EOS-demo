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
int shop_service(int fd);
void errexit(const char *content, ...);

void handler(int signum) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

char food[6][20] = {"cookie", "cake", "tea", "boba", "fried-rice", "Egg-drop-soup"};
int price[6] = {60, 80, 40, 70, 120, 50};
struct order{
    int distant;
    int amount_list[2];
};

int main(int argc, char *argv[]) {

    struct sockaddr_in fsin;   /* the address of a client */
    int	alen;	               /* length of client's address */
    int	msock;	               /* master server socket */
    int	ssock;	               /* slave server socket */
    if(argc!=2) errexit("usage: hw2 [port]\n");

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
                exit(shop_service(ssock));
            default:	/* parent */
                close(ssock);
				printf("client PID:%d\n", pid);
                break;
            case -1:
                errexit("fork: %s\n", strerror(errno));
        }
    }
}

void myprintf(int fd, char *str, int strl){
    write(fd, str, strl);
    write(fd, "\0", 1);
}

void show_list(int fd){
    myprintf(
        fd, 
        "Dessert Shop:3km\n- cookie:60$|cake:80$\nBeverage Shop:5km\n- tea:40$|boba:70$\nDiner:8km\n- fried-rice:120$|Egg-drop-soup:50$",
        strlen("Dessert Shop:3km\n- cookie:60$|cake:80$\nBeverage Shop:5km\n- tea:40$|boba:70$\nDiner:8km\n- fried-rice:120$|Egg-drop-soup:50$")    
    );
}

int shop_service(int fd){

    // dup2(fd, STDOUT_FILENO);

    char buf[1024], res[1024], cmd[100], item[100], str[1024];
    int retval, amount, i;
    struct order temp_order;
    temp_order.distant = -1;
    temp_order.amount_list[0] = 0;
    temp_order.amount_list[1] = 0;
    
    while(retval = read(fd, buf, 1024)){
        if(retval < 0 ) continue;
        i = 0;
        while(buf[i] != '\r' && buf[i] != '\n' && buf[i] != '\0') i++;
        // printf("i = %d\n", i);
        buf[i] = '\0';
        // printf("buf[i] %d %d %d\n",buf[i-1], buf[i], buf[i+1]);
        // printf("read raw => |%s|\n", buf);

        retval = sscanf(buf, "%s %s %d", cmd, item, &amount);
        // printf("cmd => |%s|\n", cmd);
        // printf("item => |%s|\n", item);
        // printf("amount => |%d|\n", amount);

        if(retval == 1 && !strcmp((const char *)cmd, "confirm")){
            if(temp_order.distant == -1){
                myprintf(fd, "Please order some meals", strlen("Please order some meals"));
                continue;
            }
            myprintf(fd, "Please wait a few minutes...", strlen("Please wait a few minutes..."));
            switch (temp_order.distant){
                case 2: sleep(3);
                case 1: sleep(2);
                case 0: sleep(3);
            }
            int total = temp_order.amount_list[0] * price[temp_order.distant*2] + temp_order.amount_list[1] * price[temp_order.distant*2+1];
            memset(str, 0, 1024);
            sprintf(str, "Delivery has arrived and you need to pay %d$", total);
            myprintf(fd, str, strlen(str));
        }
        else if(retval == 1 && !strcmp((const char *)cmd, "cancel")) return 0;
        else if(retval == 2 && !strcmp((const char *)cmd, "shop")) { show_list(fd); continue;}
        else if(retval == 3){
            for(i=0;i<6;i++)
                if(!strcmp((const char *)food[i], (const char *)item)) break;
            
            if(i == 6) continue;

            if(temp_order.distant == -1) temp_order.distant = i/2;
            if(temp_order.distant == i/2){
                temp_order.amount_list[i%2] += amount;
                memset(res, 0, 1024);
                if(temp_order.amount_list[1]){
                    sprintf(res, "%s %d", food[temp_order.distant*2+1], temp_order.amount_list[1]);
                }
                if(temp_order.amount_list[1] && temp_order.amount_list[0]){
                    sprintf(res, "|%s", strdup(res));
                }
                if(temp_order.amount_list[0]){
                    sprintf(res, "%s %d%s", food[temp_order.distant*2], temp_order.amount_list[0], strdup(res));
                }
            }
            memset(str, 0, 1024);
            sprintf(str, "%s", res);
            myprintf(fd, str, strlen(str));
            
        }
    }
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