#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <queue>
#include <sstream>
#include <fstream>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <cerrno>
#include <sys/unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/time.h>

#define QLEN 50
#define BUF_SIZE 1024
using namespace std;

char food[6][20] = {"cookie", "cake", "tea", "boba", "fried-rice", "Egg-drop-soup"};
int price[6] = {60, 80, 40, 70, 120, 50};

typedef struct order{
    int distant;
    int amount_list[2];
}order;

typedef struct timer{
    short deliverA;
    short deliverB;
}timer;

void timer_thread();
void do_deliver(int fd, int total);
void show_list(int fd);
void print_to_client(int fd, string str);
void quit_handler(int sig);

int passivesock(const char * port, int qlen);
int shop_service(int fd);
void errexit(const char *content, ...);

uint deliver_count=0, income=0;

mutex pf_mutex, mtimer_mutex, gv_mutex;
timer mtimer;

int main(int argc, char *argv[]) {

    queue <thread> thread_arr;
    struct sockaddr_in fsin;   /* the address of a client */
    int	alen;	               /* length of client's address */
    int	msock;	               /* master server socket */
    int	ssock;	               /* slave server socket */

    if(argc!=2) errexit("usage: hw3 [port]\n");

    msock = passivesock(argv[1], QLEN);

    signal(SIGINT, quit_handler);

    thread new_client(timer_thread);

    while (1) {
        alen = sizeof(fsin);
        ssock = accept(msock, (struct sockaddr *) &fsin, (socklen_t*)&alen);
        if (ssock < 0) {
            if (errno == EINTR) continue;
            errexit("accept: %s\n", strerror(errno));
        }

        thread new_client(shop_service, ssock);
        thread_arr.push(move(new_client));
    }
}

int shop_service(int fd){

    char buf[BUF_SIZE] = {0}, res[100];

    int retval, i;
    order temp_order;

    temp_order.distant = -1;
    temp_order.amount_list[0] = 0;
    temp_order.amount_list[1] = 0;

    while(retval = read(fd, buf, BUF_SIZE)){
        if(string(buf) == ""){
            printf("retval = %d\n", retval);
            errexit("NULL str");
        }
            
        // printf("FD %d: buf_len = %ld\n", fd, strlen(buf));
        stringstream ss;
        string cmd, item;
        int amount=0, buf_cut = 0;
        ss << buf;
        ss >> cmd >> item >> amount;
        
        memset(buf, 0, BUF_SIZE);

        printf("FD %d: parser => |%s|\n", fd, buf); 

        if(cmd  == "confirm"){
            if(temp_order.distant == -1){
                string str("Please order some meals");
                print_to_client(fd, str);
                //printf("FD %d: No order\n", fd);
                continue;
            }

            int wait_time = 0, eta;
            switch (temp_order.distant){
                case 2: wait_time+=3;
                case 1: wait_time+=2;
                case 0: wait_time+=3;
            }

            mtimer_mutex.lock();
            if(mtimer.deliverA > mtimer.deliverB)
                eta = mtimer.deliverB + wait_time;
            else
                eta = mtimer.deliverA + wait_time;
            mtimer_mutex.unlock();

            if(eta >= 30){
                string str("Your delivery will take a long time, do you want to wait?");
                print_to_client(fd, str);
                //printf("FD %d: Qz\n", fd);

                memset(buf, 0, BUF_SIZE);
                while(string(buf) == ""){
                    recv(fd, buf, sizeof(buf), 0);
                }
                printf("FD %d: long wait => |%s|\n", fd, buf);
                if(strcmp(buf, "Yes"))
                    return 0;
            }
            else{
                string str("Please wait a few minutes...");
                print_to_client(fd, str);
                //printf("FD %d: Plz wait\n", fd);
            }

            mtimer_mutex.lock();
            if(mtimer.deliverA > mtimer.deliverB){
                mtimer.deliverB += wait_time;
                //printf("FD %d: add B timer => |%d|\n", fd, wait_time);
                wait_time = mtimer.deliverB;
            }
            else{
                mtimer.deliverA += wait_time;
                //printf("FD %d: add A timer => |%d|\n", fd, wait_time);
                wait_time = mtimer.deliverA;
            }
            mtimer_mutex.unlock();

            printf("FD %d: sleep for %d\n", fd, wait_time);
            this_thread::sleep_for(chrono::seconds(wait_time));
            int order_total = temp_order.amount_list[0] * price[temp_order.distant * 2] + temp_order.amount_list[1] * price[temp_order.distant * 2 + 1];
            do_deliver(fd, order_total);
            return 0;
        }
        else if(cmd == "cancel") return 0;
        else if(cmd == "shop") show_list(fd);
        else if(cmd == "order") {
            for(i=0;i<6;i++)
                if(!strcmp((const char *)food[i], item.c_str())) break;
            if(i == 6){
                printf("FD %d: no such item  => |%s|\n", fd, item.c_str());
                return 0;
            }

            if(temp_order.distant == -1) temp_order.distant = i/2;
            if(temp_order.distant == i/2){
                temp_order.amount_list[i%2] += amount;
                memset(res, 0, 100);
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
            string str(res);
            print_to_client(fd, str);
            // printf("FD %d: order %s\n", fd, str.c_str());
        }
        else{
            printf("FD %d:worng cmd\n", fd);
            return 0;
        } 
    }
    return 0;
}

void timer_thread(){
    while(1){
        mtimer_mutex.lock();
        if(mtimer.deliverB || mtimer.deliverA){
            if(mtimer.deliverA > 0) mtimer.deliverA--;
            if(mtimer.deliverB > 0) mtimer.deliverB--;
            if(mtimer.deliverB || mtimer.deliverA) printf("A now:%d, B now:%d\n", mtimer.deliverA, mtimer.deliverB);
        }
        mtimer_mutex.unlock();
        this_thread::sleep_for(chrono::seconds(1));
    }
}

void do_deliver(int fd, int total){

    char tmp[100] = {0};
    sprintf(tmp, "Delivery has arrived and you need to pay %d$", total);
    print_to_client(fd, string(tmp));
    printf("FD %d: Delived\n", fd);
    gv_mutex.lock();
    income += total;
    deliver_count++;
    gv_mutex.unlock();
    close(fd);
}

void quit_handler(int sig){
    ofstream out("result.txt");
    out << "customer: " << deliver_count << endl;
    out << "income: " << income << "$" << endl;
    exit(0);
}

void print_to_client(int fd, string str){
    send(fd, str.c_str(), str.length()+1, 0);
}

void show_list(int fd){
    string str("Dessert Shop:3km\n- cookie:60$|cake:80$\nBeverage Shop:5km\n- tea:40$|boba:70$\nDiner:8km\n- fried-rice:120$|Egg-drop-soup:50$");
    print_to_client(fd, str);
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