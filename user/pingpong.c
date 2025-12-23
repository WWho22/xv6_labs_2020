#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char const *argv[])
{
    if (argc > 1)
    {
        printf("didn't need overmore args\n");
    }
    int parent_2_child_pipe[2];
    int child_pipe_2_parent[2];

    if ((pipe(parent_2_child_pipe) < 0)||(pipe(child_pipe_2_parent) < 0))
    {
        printf("pipe fail\n");
        exit(-1);
    }
    int pid = fork();
    if (pid < 0)
    {
        printf("fork error");
        exit(-1);
    }
    else if (pid == 0)
    {
        //子进程
        char* byte = "pong";
        //子进程用不到父向子管道的写端
        close(parent_2_child_pipe[1]);
        //子进程用不到子向父管道的读端
        close(child_pipe_2_parent[0]);

        if(read(parent_2_child_pipe[0],byte,4) != -1)
        {
            close(parent_2_child_pipe[0]);
            write(child_pipe_2_parent[1],byte,4);
            close(child_pipe_2_parent[1]);
            printf("%d: received ping\n",getpid());
            exit(0);
        }
        else
        {
            close(parent_2_child_pipe[0]);
            close(child_pipe_2_parent[1]);
            printf("%d: didn't receive ping\n",getpid());
            exit(-1);
        }
    }
    else
    {
        //父进程
        char* byte = "ping";
        //父进程用不到父向子管道的读端
        close(parent_2_child_pipe[0]);
        //父进程用不到子向父管道的写端
        close(child_pipe_2_parent[1]);

        if (write(parent_2_child_pipe[1],byte,4) < 0)
        {
            printf("write error");
            close(parent_2_child_pipe[1]);
            exit(-1);
        }
        wait(0);
        if (read(child_pipe_2_parent[0],byte,4) > 0)
        {
            printf("%d: received pong\n",getpid());
            close(child_pipe_2_parent[0]);
        }
        else
        {
            close(child_pipe_2_parent[0]);
            printf("%d: didn't receive pong\n",getpid());
            exit(-1);
        }
        
    }
    exit(0);
}

// #include "kernel/types.h"
// #include "user/user.h"

// int 
// main(int argc, char** argv ){
//     int pid;
//     int parent_fd[2];
//     int child_fd[2];
//     char buf[20];
//     //为父子进程建立管道
//     pipe(child_fd); 
//     pipe(parent_fd);

//     // Child Progress
//     if((pid = fork()) == 0){
//         close(parent_fd[1]);
//         read(parent_fd[0],buf, 4);
//         printf("%d: received %s\n",getpid(), buf);
//         close(child_fd[0]);
//         write(child_fd[1], "pong", sizeof(buf));
//         exit(0);
//     }
//     // Parent Progress
//     else{
//         close(parent_fd[0]);
//         write(parent_fd[1], "ping",4);
//         close(child_fd[1]);
//         read(child_fd[0], buf, sizeof(buf));
//         printf("%d: received %s\n", getpid(), buf);
//         exit(0);
//     }
    
// }
