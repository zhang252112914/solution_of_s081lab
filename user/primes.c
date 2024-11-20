#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

__attribute__((noreturn))
void filter(int pip){
    int prime;
    if(read(pip, &prime, sizeof(prime)) <= 0){
        close(pip);
        exit(0); //when there is  no number in the pipe, it should end
    }

    //the first num of the pip defintely a prime
    printf("prime %d\n", prime);

    int pipe_next[2];
    pipe(pipe_next);

    if(fork() == 0){
        close(pipe_next[1]);
        filter(pipe_next[0]);
        close(pipe_next[0]);
    }
    else{//father process
        close(pipe_next[0]);
        int num;
        while(read(pip, &num, sizeof(num))>0){
            if(num % prime != 0){
                write(pipe_next[1], &num, sizeof(num));
            }
        }
        close(pipe_next[1]);
        wait(0);
    }
    close(pip);
    exit(0);
}

int main(){
    int pip[2];
    pipe(pip);

    if(fork() == 0){
        close(pip[1]);
        filter(pip[0]);
        close(pip[0]);
    }
    else{
        close(pip[0]);
        for(int num = 2; num <= 35; num++){
            write(pip[1], &num, sizeof(num));
        }
        close(pip[1]);
        wait(0);
    }
    exit(0);
}

// //user/primes.c
// #include "kernel/types.h"
// #include "kernel/stat.h"
// #include "user/user.h"

// #define SIZE 34

// void recur(int p[2]) {
//     int primes, nums;
//     int p1[2];

//     close(0); // 0的复用
//     dup(p[0]);
//     close(p[0]);
//     close(p[1]);

//     if (read(0, &primes, 4)) {
//         printf("prime %d\n", primes); // 打印由父进程传来的第一个数字

//         pipe(p1);
//         if (fork() == 0) {
//             recur(p1); // 由子进程筛选下一批质数
//         } // 思考：考虑子进程已经在读、但是父进程还没写完的情况，子进程会等吗，还是报错呢？
//         else {
//             while (read(0, &nums, 4)) { // 从父进程读取数据
//                 if (nums % primes != 0) { // 筛查，将符合条件的数字传给子进程
//                     write(p1[1], &nums, 4);
//                 }
//             }
//             close(p1[1]);
//             close(0);
//             wait(0);
//         }
//     } else {
//         close(0); // 递归出口：若父进程无数据输入，则说明筛查完毕
//     }
//     exit(0);
// }

// int main() {
//     int p[2];
//     pipe(p);
//     for (int i = 2; i < SIZE + 2; ++i) {
//         write(p[1], &i, 4);
//     }
//     if (fork() == 0) {
//         recur(p);
//     } else {
//         close(p[1]);
//         wait(0);
//     }
//     exit(0);
// }