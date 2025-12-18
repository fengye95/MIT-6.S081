#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int 
main(int argc, char *argv[])
{
  if (argc != 1) {
    printf("Usage: pagetable\n");
    exit(1);
  }
  int ret = pagetable();
  if(ret < 0){
    printf("pagetable failed\n");
    exit(1);
  }
  exit(0);
}