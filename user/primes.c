#include "kernel/types.h"
#include "user.h"

__attribute__((noreturn))
void sieve(int lPipe[]){
    close(lPipe[1]);
    int p = 0;
    if(read(lPipe[0], &p, sizeof(p)) == 0) exit(0); //the pipe is empty
    printf("prime %d\n", p);

    int rPipe[2];
    pipe(rPipe);

    int pid = fork();
    if(pid == 0){
        close(lPipe[0]);
        sieve(rPipe);
    }
    else{
        close(rPipe[0]);
        int x = 0;
        while(read(lPipe[0], &x, sizeof(x)) > 0){ //read to the end
            if(x % p != 0) write(rPipe[1], &x, sizeof(x));
        }
        close(lPipe[0]);
        close(rPipe[1]);
        wait(0);
    }
    exit(0);
}

int main(int argc, char* argv[]){
    int p[2];
    pipe(p);

    int pid = fork();
    if(pid == 0) sieve(p);
    else{
        close(p[0]);
        for(int i = 2; i <= 35; ++i)
            write(p[1], &i, sizeof(i)); //pass integer through binary
        close(p[1]);
        wait(0);
    }

    exit(0);
}