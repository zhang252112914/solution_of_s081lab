#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]){
    if (argc <= 1){
        printf("sleep need parameter\n");
        exit(0);
    }

    int wait = atoi(argv[1]);
    sleep(wait);
    exit(0);
}