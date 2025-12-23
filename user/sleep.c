#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char const *argv[])
{
    if (argc<2)
    {
        printf("ERROR:less than 2 args\n");
        exit(-1);
    }
    int sleep_time = atoi(argv[1]);
    if (sleep(sleep_time) < 0)
    {
        printf("sleep fail\n");
        exit(-1);
    }
    printf("sleep complete\n");

    // return 0;
    exit(0);
}
