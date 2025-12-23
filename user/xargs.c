#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"
#define MAXSIZE 16

int main(int argc, char *argv[])
{
    char* xarg[MAXARG];
    char buf[MAXSIZE];
    char* p = buf;
    int xarg_num = argc-1;
     //获取标准输入的参数
     read(0,buf,MAXSIZE);
    //  printf("get stdin args:%s",buf);
    //dubug了一下，标准输入的内容的最后一个字符是'\n'
     //获取自身当前已有的参数
     for (int i = 0; i < xarg_num; i++)
     {
        xarg[i] = argv[i+1];
        // printf("args%d:%s\n",i,xarg[i]);
     }
     //将标准输入的参数和自身的参数拼接成要执行的命令及其参数
     for (int i = 0; i < MAXSIZE; i++)
     {
        if(buf[i]  == '\n')
        {
            int pid = fork();
            if (pid < 0)
            {
                printf("fork error\n");
                exit(-1);
            }
            else if(pid == 0)
            {
                //子进程
                // printf("xarg_num:%d\n",xarg_num);
                buf[i] = '\0';
                xarg[xarg_num] = p;
                // printf("xarg[%d]:%s\n",xarg_num,xarg[xarg_num]);
                xarg_num++;
                xarg[xarg_num] = 0;
                xarg_num++;
                //创建子进程执行命令，父进程等待子进程执行完毕
                exec(xarg[0],xarg);
                exit(0);
            }
            else
            {
                //父进程
                p = &buf[i+1];
                wait(0);
            }
        }
     }
    exit(0);
}
