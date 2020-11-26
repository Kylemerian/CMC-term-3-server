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
/*
TODO 
add shutdown for  leavers
*/
int shared = 0;

typedef struct usr{
    int fd;
    struct usr * next;
} 
usr;

int cmdFromPlayer(int fd, usr * list)
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
    if(res == -1)
        return 0;
    return 1;
}

usr * addUsr(usr * list, int fd)
{
    usr * tmp = malloc(sizeof(* tmp));
    tmp -> next = list;
    tmp -> fd = fd;
    dprintf(fd, "welcome, user №%d\n", fd);
    return tmp;
}

usr * deleteUsr(usr * list, int fd)
{
    usr * node = list;
    if(node -> fd == fd){
        list = list -> next;
        free(node);
    }
    else{
        while(node -> next -> fd != fd)
            node = node -> next;
        if(node -> next -> next == NULL){
            free(node -> next);
            node -> next = NULL;
        }
        else{
            usr * tmp = node -> next;
            node -> next = node -> next -> next;
            free(tmp);
        }
    }
    return list;
}

void freeMem(usr * list)
{
    usr * tmp;
    while(list){
        tmp = list;
        list = list -> next;
        free(tmp);
    }
}

usr * updateUsrs(usr * players, usr * disconn)
{
    while(disconn){
        players = deleteUsr(players, disconn -> fd);
        disconn = disconn -> next;
    }
    freeMem(disconn);
    return players;
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
    usr * disconn = NULL;
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
                res = cmdFromPlayer(fd, players);
                if(res == 0)
                    disconn = addUsr(disconn, fd);;
            }
            tmp = tmp -> next;
        }
        players = updateUsrs(players, disconn);
        printUsrs(players);/**/
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