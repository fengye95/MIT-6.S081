#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

/*
路径 -> 文件名/目录名 （取出从左往右最后一个‘/’后面的内容）

文件
fmtname("/home/user/file.txt") → "file.txt" ✅

目录  
fmtname("/usr/local/bin") → "bin" ✅

当前目录下的文件
fmtname("script.sh") → "script.sh" ✅

以斜杠结尾（目录）
fmtname("/tmp/") → "" ✅ (空字符串，合理)

fmtname3 -> 方案2：动态分配内存

非纯函数害人不浅
*/
char*
fmtname3(char *path)
{
  char *buf = malloc(DIRSIZ+1);
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
  return buf;
}


void
find(char *path, char *filename)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if((fd = open(path, 0)) < 0){
    fprintf(2, "ls: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type){
  case T_FILE:

    if (!strcmp(fmtname3(path), filename)) {
        printf("%s\n", path);
    }
    break;

  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf("find: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    // 解析当前目录
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;

      // 如果是 . 或 .. 跳过， 防止无限递归
      if (!strcmp(de.name, ".") || !strcmp(de.name, ".."))
        continue;

      // 拼接下一级路径
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;

      if(stat(buf, &st) < 0){
        printf("find: cannot stat %s\n", buf);
        continue;
      }
      
      find(buf, filename);
    }
    break;
  }
  close(fd);
}

int
main(int argc, char *argv[])
{
  if(argc < 3){
    fprintf(2, "Usage: find <path> <filename>\n");
    exit(1);
  }

  char *path = argv[1];
  char *filename = fmtname3(argv[2]);
  find(path, filename);

  exit(0);
}
