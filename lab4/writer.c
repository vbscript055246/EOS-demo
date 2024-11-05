#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[]){

    if(argc != 2) {
        fprintf(stderr, "Usage: ./writer <Name>");
        exit(EXIT_FAILURE);
    }

    int fd, i=0;

    if((fd = open("/dev/mydev", O_RDWR)) < 0){
        printf("Open failed\n");
        exit(-1);
    }
    int length = 0;
    while (argv[1][length]!='\0') length++;
    // strlen(argv[1][i]);
    // int length = 4;
    // printf("%d\n", length);
    while(1){
        // printf("%c\n", argv[1][i%length]);
        write(fd, (argv[1] + i%length), 1);
        sleep(1);
        i++;
    }
    close(fd);
}