#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  if(argc != 1){
    fprintf(2, "Usage: pingpong\n");
    exit(1);
  }

  int pf[2], ps[2];
  pipe(pf);
  pipe(ps);

  // 需要正确关闭pf[1], ps[1]。 否则会阻塞read()或读取不成功
  if (fork() == 0) {
    // 子进程 fork 返回 0
    close(ps[1]);

    char *buf = malloc(1);
    read(ps[0], buf, 1);
    printf("%d: received ping\n", getpid());

    write(pf[1], buf, 1);
    close(pf[1]);
    exit(0);
  }
  else {
    // 父进程
    close(pf[1]);

    char *buf = malloc(1);
    write(ps[1], buf, 1);
    close(ps[1]);

    read(pf[0], buf, 1);
    printf("%d: received pong\n", getpid());
    exit(0);
  }

  exit(0);
}
