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
add shutdown for leavers
*/
int shared = 0;

typedef struct usr{
    int fd;
    char * buf;
    int bufsize;
    int cnt;
    struct usr * next;
} 
usr;

void printUsrs(usr * list)
{
    usr * tmp = list;
    while(tmp){
        printf("%d\n", tmp -> fd);
        tmp = tmp -> next;
    }
}

usr * findUsr(int fd , usr * list)
{
    usr * tmp = list;
    while(tmp -> fd != fd)
        tmp = tmp -> next;
    return tmp;
}

char * extendBuf(usr ** user)
{
    char * newBuf = malloc(2 * ((*user) -> cnt));
    strncpy(newBuf, (*user) -> buf, (*user) -> bufsize - 1);
    (*user) -> bufsize *= 2;
    free((*user) -> buf);
    return newBuf;
}

void execCmd(usr * user)
{
    int i;
    for(i = 0; i < user -> cnt; i++){
        if(user -> buf[i] == ' ' || user -> buf[i] == '\t' || user -> buf[i] == '\r')
            user -> buf[i] = 0;
    }

    /**/
    i = 0;
    while(i < user -> cnt){
        printf("%s\n", &(user -> buf[i]));
        while(user -> buf[i] != 0)
            i++;
        while(user -> buf[i] == 0)
            i++;
    }
}

int cmdFromPlayer(int fd, usr * list)
{
    usr * user = findUsr(fd, list);
    if(user -> cnt == user -> bufsize - 1)
        user -> buf = extendBuf(&user);

    //printf("usr = %d; %s; %d\n", user -> fd, user -> buf, user -> cnt);
    int res = read(fd, &(user -> buf[user -> cnt]), 1);
    user -> cnt ++;
    
    if(user -> buf[user -> cnt - 1] == '\n'){
        execCmd(user);
        user -> buf[user -> cnt] = 0;
        dprintf(fd, "buff was changed by %d and now = %s\n", fd, user -> buf);
        user -> cnt = 0;
    }
    if(res == 0)
        return 0;
    return 1;
}

usr * addUsr(usr * list, int fd)
{
    usr * tmp = malloc(sizeof(* tmp));
    tmp -> next = list;
    tmp -> fd = fd;
    tmp -> bufsize = 16;
    tmp -> cnt = 0;
    tmp -> buf = calloc(tmp -> bufsize, 1);
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
            /**/
            printf("thats fd = %d\n", tmp -> fd);
            free(tmp);
        }
        printf("DBG\n");
        printUsrs(node);/*DBG*/
        return node;
    }
    printf("DBG\n");
    printUsrs(list);/*DBG*/
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
    usr * tmp = disconn;
    while(tmp){
        shutdown(tmp -> fd, 2);
        close(tmp -> fd);
        players = deleteUsr(players, tmp -> fd);
        tmp = tmp -> next;
    }
    freeMem(disconn);
    return players;
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
                if(res == -1){/**/
                    disconn = addUsr(disconn, fd);
                    printf("was here\n");
                }
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