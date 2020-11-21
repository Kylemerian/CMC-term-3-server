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

void processing(int ls)
{
    while(1){
        fd_set readfds;
        FD_ZERO(&readfds);
    }
}

int main()
{
    int ls, port;
    ls = socket(AF_INET, SOCK_STREAM, 0);
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
    void processing(ls);
    return 0;
}