#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void find(char* path,char* file_name)
{
   char buf[512], *p;
   int fd;
   struct dirent de;
   struct stat st;
   
    if((fd = open(path, 0)) < 0)
    {  
      // 0表示只读
      fprintf(2, "find: cannot open %s\n", path);
      return;
    }

    if (fstat(fd,&st))
    {
        //无法读取文件的状态信息
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch(st.type)
    {
    //如果传入目录的文件种类是文件，提示传入的目录错误
    case T_FILE:
    printf("find: it is not a directory\n");
    break;

    case T_DIR:
    //检测当前路径长度加上文件名最长长度是否超出缓冲区长度
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf)
    {
      printf("ls: path too long\n");
      break;
    }
    //将当前路径存到buf中，并在路径后面加上斜杆以代表目录下有子目录和文件
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    //读取目录下所有子目录和文件
    while(read(fd, &de, sizeof(de)) == sizeof(de))
    {
      //如果给定目录下的子目录为空目录
      if(de.inum == 0)
        continue;
      //如果子目录中遇到.或者..，直接跳过，防止无限递归
      if ((strcmp(de.name,".") == 0)||(strcmp(de.name,"..") == 0))
      {
        continue;
      }
      //将文件名或目录名添加到/后面，并加上字符串终止符0
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      //通过绝对路径来获取文件或目录的文件状态
      if(stat(buf, &st) < 0){
        printf("ls: cannot stat %s\n", buf);
        continue;
      }
      if (st.type == T_FILE)
      {
        //如果该文件状态中类型为文件，且名称与目标名称相同
        if (strcmp(p,file_name) == 0)
        {
          printf("%s\n",buf);
        }
      }
      else if (st.type == T_DIR)
      {
        //如果该文件状态中类型为目录，递归调用find
        find(buf,file_name);
      }
    }
    break;
  }
    close(fd);
    return;

}

int main(int argc, char *argv[])
{
    if ( argc < 3)
    {
        printf("Missing parameter\n");
        exit(-1);
    }
    find(argv[1],argv[2]);
    exit(0);
}