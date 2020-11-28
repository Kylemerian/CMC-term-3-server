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

enum actions{
    buy,
    sell,
    build,
    produce,
    end
};

enum resources{
    money,
    raw,
    production,
    factories
};

typedef struct gm{
    int lvl;
    int isStarted;
    int players;
    int month;
    int priceRaw;
    int priceProd;
}
gm;

typedef struct usr{
    int fd;
    char * buf;
    int bufsize;
    int cnt;
    int resources[4];
    int nums[5];
    int isBankrupt;
    struct usr * next;
} 
usr;

gm * init(gm * game)
{
    gm * tmp = malloc(sizeof(* tmp));
    tmp -> lvl = 3;
    tmp -> isStarted = 0;
    tmp -> players = 0;
    tmp -> priceProd = 5500;
    tmp -> priceRaw = 500;
    tmp -> month = 1;
    return tmp;
}

void printUsrs(usr * list)
{
    usr * tmp = list;
    while(tmp){
        printf("%d %d\n", tmp -> fd, tmp -> resources[money]);
        tmp = tmp -> next;
    }
}

void printHelp(int fd)
{
    const char s1[] = ">> buy [num]       buy [num] raw";
    const char s2[] = ">> sell [num]      sell [num] production";
    const char s3[] = ">> produce [num]   produce [num] production";
    const char s4[] = ">> build [num]     build [num] factories";
    const char s5[] = ">> end turn        end current turn";
    dprintf(fd, "%s\n%s\n%s\n%s\n%s\n", s1, s2, s3, s4, s5);
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

int countWords(char * s, int maxlen)
{
    int i = 0;
    int cnt = 0;
    while(i < maxlen){
        if(s[i] == 0)
            while(s[i] == 0)
                i++;
        cnt++;
        i++;
        while(s[i] != 0)
            i++;
    }
    return cnt - 1;
}

void isCorrect(char ** arg, usr * user, gm * game)
{
    int i;
    const char * cmds[6];
    cmds[0] = "buy";
    cmds[1] = "sell";
    cmds[2] = "build";
    cmds[3] = "produce";
    cmds[4] = "end";
    cmds[5] = "turn";
    /**/printf("User %d : %s %s\n", user -> fd - 3, arg[0], arg[1]);
    for(i = 0; i < 4; i++){
        if(!strcmp(arg[0], cmds[i])){
            int res = sscanf(arg[1], "%d", &user -> nums[i]);
            if(res && !(user -> nums[end]))//
                dprintf(user -> fd, ">> Your request accepted : %s %d\n", arg[0], user -> nums[i]);
            else
                dprintf(user -> fd, ">> Incorrect input OR you end turn before\n");
            break;
        }
        else if(i == 3 && strcmp(arg[0], cmds[4]))
            dprintf(user -> fd, ">> Wrong command, print \"help\" for help\n");
    }
    if(!strcmp(arg[0], cmds[4])){
        if(!strcmp(arg[1], cmds[5]))
            user -> nums[end] = 1;
        else
            dprintf(user -> fd, ">> Maybe you would like \"end turn\"?\n");
    }
}

void execCmd(usr * user, gm * game)
{
    int i;
    char * arg[2];
    for(i = 0; i < user -> cnt; i++){
        if(user -> buf[i] == ' ' || user -> buf[i] == '\t' || user -> buf[i] == '\r')
            user -> buf[i] = 0;
    }
    int numWords = countWords(user -> buf, user -> cnt);
    if(numWords == 1){
        i = 0;
        while(user -> buf[i] == 0)
            i++;
        arg[0] =  &(user -> buf[i]);
        if(!strcmp("help", arg[0]))
            printHelp(user -> fd);
        else
            dprintf(user -> fd, ">> Wrong command, print \"help\" for help\n");
    }
    else if(numWords == 2){
        i = 0;
        while(user -> buf[i] == 0)
            i++;
        arg[0] =  &(user -> buf[i]);
        while(user -> buf[i] != 0)
            i++;
        while(user -> buf[i] == 0)
            i++;
        arg[1] = &(user -> buf[i]);
        isCorrect(arg, user, game);
    }
    else{
        dprintf(user -> fd, ">> Wrong command, print \"help\" for help\n");
    }

}

int cmdFromPlayer(int fd, usr * list, gm * game)
{
    usr * user = findUsr(fd, list);
    if(user -> cnt == user -> bufsize - 1)
        user -> buf = extendBuf(&user);

    int res = read(fd, &(user -> buf[user -> cnt]), 1);
    user -> cnt++;
    if(user -> buf[user -> cnt - 1] == '\n'){
        user -> buf[user -> cnt] = 0;
        execCmd(user, game);
        user -> cnt = 0;
    }
    if(res == 0)
        return 0;
    return 1;
}

usr * addUsr(usr * list, int fd)
{
    int i;
    usr * tmp = malloc(sizeof(* tmp));
    tmp -> next = list;
    tmp -> fd = fd;
    tmp -> bufsize = 16;
    tmp -> cnt = 0;
    tmp -> resources[money] = 10000;
    tmp -> resources[raw] = 4;
    tmp -> resources[factories] = 2;
    tmp -> resources[production] = 2;
    tmp -> isBankrupt = 0;
    for(i = 0; i < 5; i++)
        tmp -> nums[i] = 0;
    tmp -> buf = calloc(tmp -> bufsize, 1);
    dprintf(fd, ">> Welcome, user №%d\n", fd - 3);
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
        return node;
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

int endTurn(usr * players){
    usr * tmp = players;
    int res = 1;
    while(tmp){
        res *= tmp -> nums[end];
        tmp = tmp -> next;
    }
    return res;
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

usr * changeValues(usr * users)
{
    usr * tmp = users;
    while(tmp){
        tmp -> nums[end] = 0;
        tmp = tmp -> next;
    } 
    tmp = users;
    while(tmp){
        tmp -> resources[money] -= (300 * tmp -> resources[raw] + 500 * 
            tmp -> resources[production] + 1000 * tmp -> resources[factories]);
        tmp = tmp -> next;
    }
    return tmp;
}

void updateGame(gm * game, usr * users)
{
    users = changeValues(users);
}

void printTurnEach(usr * players, int fd)
{
    const char s1[] = ">> User #";
    const char s2[] = ">>   Money : ";
    const char s3[] = ">>   Raw : ";
    const char s4[] = ">>   Production : ";
    const char s5[] = ">>   Factories : ";
    usr * tmp = players;
    while(tmp){
        dprintf(fd, "%s%d\n%s%d\n%s%d\n%s%d\n%s%d\n", s1, tmp -> fd - 3, s2, 
            tmp -> resources[money], s3, tmp -> resources[raw], s4, 
            tmp -> resources[production], s5, tmp -> resources[factories]);
        tmp = tmp -> next;
    }
}

void printTurn(usr * players, gm * game)
{
    usr * tmp = players;
    while(tmp){
        printTurnEach(players, tmp -> fd);
        tmp = tmp -> next;
    }
}

void processing(int ls)
{
    int max_d, fd, res;
    gm * game = NULL;
    usr * players = NULL;
    usr * tmp = NULL;
    usr * disconn = NULL;
    game = init(game);
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
            perror(">> Err in select()\n");
        if(FD_ISSET(ls, &readfds)){
            fd = accept(ls, NULL, NULL);
            if (fd != -1)
                players = addUsr(players, fd);
            else
                perror(">> Err in accept()\n");
        }
        tmp = players;
        disconn = NULL;
        while(tmp){
            fd = tmp -> fd;
            if(FD_ISSET(fd, &readfds)){
                res = cmdFromPlayer(fd, players, game);
                if(res == 0){
                    disconn = addUsr(disconn, fd);
                    printf(">> Smbd disconnected...\n");
                }
            }
            tmp = tmp -> next;
        }
        if(endTurn(players)){
            updateGame(game, players);
            printTurn(players, game);
        }
        players = updateUsrs(players, disconn);
    }
}

int main(int argc, char ** argv)
{
    int opt = 1;
    int ls = socket(AF_INET, SOCK_STREAM, 0);
    //int num = atoi(argv[2]);
    int port = atoi(argv[1]);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    setsockopt(ls, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (0 != bind(ls, (struct sockaddr *) &addr, sizeof(addr)))
    {
        perror(">> Smth goes wrong...\n");
        exit(1);
    }
    if (-1 == listen(ls, 5)) {
        perror(">> Smth goes wrong...\n");
        exit(1);
    }
    processing(ls);
    return 0;
}