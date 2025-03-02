#include "kernel/types.h"
#include "user.h"
#include "kernel/stat.h"
#include "kernel/fs.h"

char* fmtname(char *path){
    static char buf[DIRSIZ+1];
    char *p;

    // Find first character after last slash.
    for(p=path+strlen(path); p >= path && *p != '/'; p--)
        ;
    p++;

    // Return blank-padded name.
    if(strlen(p) >= DIRSIZ)
        return p;
    memmove(buf, p, strlen(p));
    buf[strlen(p)] = 0;
    return buf;
}

int strEqual(char* p, char* q){
    if(strlen(p) != strlen(q)) return 0;
    while(*p && *p == *q){
        ++p;
        ++q;
    }
    if(*p) return 0;
    return 1;
}

//find the file which name is 'name' in the current path
void find(char* path, char* name){ 
    struct stat st;
    struct dirent de;
    int fd;
    char buf[512];
    
    if((fd = open(path, 0)) < 0){
        fprintf(2, "find cannot open %s\n", path);
        return;
    }

    if(fstat(fd, &st) < 0){
        fprintf(2, "ls: cannot stat %s\n", path);
        close(fd);
        return;
    }

    if(st.type == T_DIR){ //directory
        if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
            printf("ls: path too long\n");
            return;
        }
        strcpy(buf, path);
        char* p = buf + strlen(buf);
        *p = '/';
        ++p;

        while(read(fd, &de, sizeof(de)) == sizeof(de)){ //read the directory entry in it
            if(de.inum == 0) continue;
            memmove(p, de.name, DIRSIZ);
            p[DIRSIZ] = 0;
            if(stat(buf, &st) < 0){
                printf("ls: cannot stat %s\n", buf);
                continue;
            }

            char* fileName = fmtname(buf);
            if(strEqual(fileName, ".") || strEqual(fileName, "..")) continue; //
            if(st.type == T_DIR){ //recursive search
                find(buf, name);
            }
            else{
                if(strEqual(fileName, name)) printf("%s\n", buf);
            }
        }
    }
    else{ //file or divice
        char* fileName = fmtname(buf);
        if(strEqual(fileName, name)) printf("%s\n", buf);
    }

    close(fd);
}

int main(int argc, char* argv[]){
    if(argc < 3){
        fprintf(2, "usage: find path fileName\n");
        exit(1);
    }

    find(argv[1], argv[2]);

    exit(0);
}