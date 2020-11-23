#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

int shared = 0;

typedef struct usr{
    int fd;
    struct usr * next;
} 
usr;

void cmdFromPlayer(int fd, usr * list)
{
    char buf[2];
    int res = read(fd, &buf, 1);
    if(res == 1 && buf[0] != '\n' && buf[0] != '\r'){
        shared = buf[0] - '0';
        usr * tmp = list;
        while(tmp){
            dprintf(tmp -> fd, "shared var was changed by %d and now = %d\n", fd, shared);
            tmp = tmp -> next;
        }
    }
}

usr * addUsr(usr * list, int fd)
{
    usr * tmp = malloc(sizeof(* tmp));
    tmp -> next = list;
    tmp -> fd = fd;
    dprintf(fd, "welcome, user №%d\n", fd);
    return tmp;
}

void printUsrs(usr * list)
{
    usr * tmp = list;
    while(tmp){
        printf("%d\n", tmp -> fd);
        tmp = tmp -> next;
    }
}

void processing(int ls)
{
    int max_d, fd, res;
    usr * players = NULL;
    usr * tmp = NULL;
    while(1){
        fd_set readfds;
        max_d = ls;
        FD_ZERO(&readfds);
        FD_SET(ls, &readfds);
        tmp = players;
        while(tmp){
            fd = tmp -> fd;
            FD_SET(fd, &readfds);
            if(fd > max_d)
                max_d = fd;
            tmp = tmp -> next;
        }
        res = select(max_d + 1, &readfds, NULL, NULL, NULL);
        if(res < 1)
            perror("Err in select()");
        if(FD_ISSET(ls, &readfds)){
            fd = accept(ls, NULL, NULL);
            if (fd != -1){
                players = addUsr(players, fd);
            }
            else{
                perror("Err in accept()");
            }
        }
        tmp = players;
        while(tmp){
            fd = tmp -> fd;
            if(FD_ISSET(fd, &readfds)){
                cmdFromPlayer(fd, players);
            }
            tmp = tmp -> next;
        }
        //printUsrs(players);
    }
}

int main(int argc, char ** argv)
{
    int ls = socket(AF_INET, SOCK_STREAM, 0);
    //int num = atoi(argv[2]);
    int port = atoi(argv[1]);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (0 != bind(ls, (struct sockaddr *) &addr, sizeof(addr)))
    {
        perror("Smth goes wrong...");
        exit(1);
    }
    if (-1 == listen(ls, 5)) {
        perror("Smth goes wrong...");
        exit(1);
    }
    processing(ls);
    return 0;
}