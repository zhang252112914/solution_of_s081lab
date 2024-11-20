// #include "kernel/types.h"
// #include "kernel/stat.h"
// #include "user/user.h"
// #include "kernel/param.h"

// int main(int argc, char* argv[]){
//     char stdIn[512];
//     int size = read(0,stdIn,sizeof(stdIn));
//     if(size < 0){
//         printf("xargs: wrong input received\n");
//         exit(0);
//     }

//     int line = 0;
//     for(int k = 0; k < size; k++){
//         if(stdIn[k] == '\\' && stdIn[k+1] == 'n'){
//             line++;
//         }
//         if(k == (size-1)){
//             if(size > 1){
//                 if(!(stdIn[k-2] == '\\' && stdIn[k-1] == 'n')){
//                     line++;
//                 }
//             }
//             else{
//                 line++;
//             }
//         }
//     }
//     int i = 0, j = 0;
//     char new_args[line][64];
//     for (int k = 0; k < size; k++){
//         new_args[i][j++] = stdIn[k];
//         if(stdIn[k] == '\\' && stdIn[k+1] == 'n'){
//             new_args[i][j-1]=0;
//             i++;
//             j=0;
//             k++;
//         }
//     }

//     char *argument[MAXARG];
//     for(int k = 0; k < argc-1; k++){
//         argument[k] = argv[k+1];
//     }

//     i = 0;
//     while(i < line){
//         argument[argc-1] = new_args[i++];
//         if(fork() == 0){
//             exec(argv[1], argument);
//             exit(0);
//         }
//         wait(0);
//     }
//     exit(0);
// }

// online version
// #include "kernel/types.h"
// #include "kernel/stat.h"
// #include "user/user.h"
// #include "kernel/fs.h"
// #include "kernel/param.h"

// int main(int argc, char *argv[])
// {
//    char buf[512];
//    char *args[MAXARG];
//    char c;
//    char *p;
//    int n;
//    int status;

//    memset(buf,0,sizeof(buf));
//    memset(args,0,sizeof(args));
//    if(argc < 2){
//        printf("need more parameter!\n");
//        exit(0);
//    }

//    /*read from the file*/
//    for(int i = 1; i < argc; ++i){
//        args[i-1] = argv[i];
//    }
//    for(;;){
//      p = buf;
//      /*read each line from the stand input*/
//      while((n = read(0,&c,1)) && c != '\n'){
//         *p = c;
//         p++;
//      }
//      *p = '\0';
//      if(p != buf){
//         args[argc-1] = buf;
//         if(fork() == 0){
//             exec(argv[1],args);
//             exit(1);
//         }
//         wait(&status);
//      }
//      if(n == 0) break;
//      if(n < 0){
//         fprintf(2, "read error\n");
//         exit(1);
//      }
//    }

//    exit(0);
// }

//sky version
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
  char buf[512];
  char *args[MAXARG];
  char c;

  if (argc < 2) {
    fprintf(2, "Usage: xargs <command>\n");
    exit(1);
  }

  for (int i = 1; i < argc; i++) {
    args[i - 1] = argv[i];
  }

  int i = 0;
  while (read(0, &c, 1) > 0) {
    if (c == '\n') {
      buf[i] = '\0';
      args[argc - 1] = buf;
      if (fork() == 0) {
        exec(args[0], args);
        fprintf(2, "xargs: exec %s failed\n", args[0]);
        exit(1);
      }
      wait(0);
      i = 0;
    } else {
      buf[i++] = c;
    }
  }

  exit(0);
}