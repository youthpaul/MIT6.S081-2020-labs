#include "kernel/types.h"
#include "user.h"

int main(int argc, char* argv[]){
    if(argc <= 1){
        fprintf(2, "usage: sleep [number]\n"); //pass no number
        exit(1);
    }

    int time = atoi(argv[1]);
    sleep(time);


    exit(0);
}