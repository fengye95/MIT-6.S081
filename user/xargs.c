#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

#define MAXLINE 100

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(2, "usage: xargs <command> [args...]\n");
        exit(1);
    }

    char *args[MAXARG];
    char line[MAXLINE];
    int offset = -1;

    // 复制基础参数
    for (int i = 1; i < argc; i++) {
        args[i - 1] = argv[i];
    }

    while (read(0, &line[++ offset], 1) > 0) {
        
        // 遇到换行符：一行结束
        if (line[offset] == '\n') {
            line[offset] = '\0';  // 结束字符串
            
            // 设置参数：基础参数 + 当前行
            args[argc - 1] = line;
            args[argc] = 0;
            
            // 执行命令
            if (fork() == 0) {
                // 子进程执行命令
                exec(args[0], args);
            }
            
            // 父进程等待
            wait(0);
            
            // 重置 offset 准备下一行
            offset = -1;
        } 
    }
    
    // 处理最后一行（如果没有换行符结束）
    if (offset >= 0) {
        line[offset + 1] = '\0';
        args[argc - 1] = line;
        args[argc] = 0;
        
        int pid = fork();
        if (pid == 0) {
            exec(args[0], args);
        }
        wait(0);
    }

    exit(0);
}