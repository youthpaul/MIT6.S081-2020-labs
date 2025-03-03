#include "kernel/types.h"
#include "kernel/param.h"
#include "user.h"

int main(int argc, char* argv[]){
    if(argc < 2){
        fprintf(2, "usage: xargs [command] {arguments...}\n");
        exit(1);
    }

    char input[512];
    read(0, input, sizeof(input));
    char* command = argv[1];

    char* l = input;
    while(*l){
        char* r = l;
        while(*r && *r != '\n') ++r; //get each input line, which is input[l : r)
        *r = 0;

        int pid = fork();
        if(pid == 0){
            char* args[MAXARG];
            int cnt = 0; //the number of arguments
            args[cnt++] = command;
            for(int i = 2; i < argc; ++i) args[cnt++] = argv[i];
            while(l < r){ //extract more the one argument in one line, such as: arg1 arg2 arg3
                if(*l == ' '){
                    ++l;
                    continue;
                }
                char* p = l;
                while(p < r && *p != ' ') ++p;
                *p = 0;
                args[cnt++] = l; //another argument
                l = ++p;
            }

            args[cnt++] = 0; //the end of arguments;
            exec(command, args);
            exit(1); //child fail to exec
        }
        else{
            wait(0);
        }
        
        l = ++r;
    }

    exit(0);
}