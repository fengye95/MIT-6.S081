#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    int pl[2], pr[2];
    pipe(pl);

    // 发送初始数据到pl[0]， exit(0) 非显式释放子进程pl管道资源
    if (fork() == 0) {
        for (int i = 2 ; i <= 35 ; i ++ ) 
            write(pl[1], &i, sizeof(int));
        exit(0);
    }
    close(pl[1]); // 关闭写端口, 保留pl[0]读端口。 防止由于写端口存在， 阻塞while(read)

    // 执行管道埃氏筛
    if (fork() == 0) {
        /*
         * 管道筛法的核心：每个进程都是一个过滤器
         * 1. 从左边读取第一个数作为质数
         * 2. 过滤掉该质数的所有倍数
         * 3. 将剩余数传递给右边
         * 4. 递归创建下一个过滤器
         */
        int num, p;

        // 递归入口点：每个过滤器都有相同的结构
        NEXT:
        
        pipe(pr);

        // 步骤1：读取当前过滤器负责的质数
        // 如果读取失败（没有更多数），说明筛法已完成
        if (read(pl[0], &p, sizeof(int)) == 0) 
            exit(0);

        printf("prime %d\n", p);
        
        // 步骤2：过滤当前质数的倍数
        // read()在遇到EOF时会返回0，循环自然结束
        while (read(pl[0], &num, sizeof(int))) {
            if (num % p != 0)
                write(pr[1], &num, sizeof(int));
        }
        
        // 步骤3：清理当前级的管道
        close(pl[0]);   // 不再从左边读取（数据已处理完）
        close(pr[1]);   // 不再向右边写入（所有数据已传递）

        // 步骤4：递归创建下一级过滤器
        if (fork() == 0) {
            // 子进程成为下一级过滤器
            // 将右边的读端变成自己的左边读端
            pl[0] = pr[0];   // 关键：继承管道
            goto NEXT;
        }
        
        // 当前进程：等待下一级过滤器完成
        close(pr[0]);   // 父进程不需要这个读端， 不写不影响正确性
        wait(0);
        exit(0);
    }

    // 主进程：等待所有子进程完成
    close(pl[0]);  // 主进程不需要读端， 不写不影响正确性
    wait(0);
    wait(0); 

    exit(0);
}
