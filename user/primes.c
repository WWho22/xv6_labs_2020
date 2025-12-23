#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


void sieve(int current_pipe)
{
    int num = 0;

    //每个循环第一次读到管道数据，该数据为当前进程的质数
    if (read(current_pipe,&num,sizeof(int)) > 0)
    {

            //当前进程的质数
            printf("prime %d\n",num);
    }
    else
    {
        printf("read error\n");
        exit(0);
    }

    //创建存储下一批待处理数据的管道
    int pipe_prime_next[2];
    if (pipe(pipe_prime_next) < 0)
    {
        printf("pipe fail\n");
        exit(-1);
    }

    int pid = fork();
    if (pid < 0)
    {
        printf("fork error\n");
        exit(-1);
    }
    else if (pid == 0)
    {
        //当前进程不需要读传入的管道
        close(current_pipe);
        //关闭写端
        close(pipe_prime_next[1]);
        //在子进程中递归调用质数筛
        sieve(pipe_prime_next[0]);
        //关闭读端
        close(pipe_prime_next[0]);
    }
    else
    {
        //关闭读端
        close(pipe_prime_next[0]);
        int handle_num = 0;
        while (read(current_pipe,&handle_num,sizeof(handle_num)))
        {
            if (handle_num % num != 0)
            {
                write(pipe_prime_next[1],&handle_num,sizeof(handle_num));
            }
        }
        close(pipe_prime_next[1]);
        wait(0);
    }
}

int main(int argc, char const *argv[])
{
    if (argc > 1)
    {
        printf("didn't need overmore args\n");
    }
    int pipe_prime_current[2];
   
    //创建管道
    if (pipe(pipe_prime_current) == -1)
    {
        printf("pipe fail\n");
        exit(-1);
    }
   
    //创建子进程
    int pid = fork();

    if (pid < 0)
    {
        printf("fork error\n");
        exit(-1);
    }
    else if (pid == 0)
    {
        //子进程
        //关闭读端
        close(pipe_prime_current[0]);
        for (int i = 2; i <= 35; i++)
        {
          write(pipe_prime_current[1],&i,sizeof(i));
        }
        //关闭写端
        close(pipe_prime_current[1]);
        exit(0);          
    }
    else
    {
        //当前父进程
        //关闭写端
        close(pipe_prime_current[1]);
        //质数筛
        sieve(pipe_prime_current[0]);
        //关闭读端
        close(pipe_prime_current[0]);
        
    }
    wait(0);
    exit(0);
}

