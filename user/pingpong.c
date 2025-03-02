#include "kernel/types.h"
#include "user.h"

int main(int argc, char* argv[]){
    int p1[2], p2[2];
    char buf[] = {'p'};

    //need two pipes
    pipe(p1); //pipe1 : parent -> child
    pipe(p2); //pipe2 : child -> parent

    int sta = fork();
    int pid = getpid();
    if(sta == 0){ //child
        printf("%d: received ping\n", pid);
        close(p1[1]);
        read(p1[0], buf, sizeof(buf));
        /* write to parent*/
        close(p2[0]);
        write(p2[1], buf, sizeof(buf));
    }
    else{ //parent
        /* write to child */
        close(p1[0]);
        write(p1[1], buf, sizeof(buf));
        close(p2[1]);
        read(p2[0], buf, sizeof(buf));
        printf("%d: received pong\n", pid);
    }

    exit(0);
}