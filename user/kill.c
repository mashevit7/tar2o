#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char **argv)
{
  if(argc < 3) {
    fprintf(2, "usage: kill (<pid> <signal>)+\n");
    exit(1);
  }
  if (argc % 2 == 0) {
    fprintf(2, "expected even number of arguments.\nusage: kill (<pid> <signal>)+\n");
    exit(2);
  }
  for (int i = 1; i < argc; i += 2) {
    int pid = atoi(argv[i]);
    int signum = atoi(argv[i + 1]);
    if (kill(pid, signum) < 0) {
      printf("iter %d: sending signal %d to process %d failed.\n", (i / 2) + 1, signum, pid);
    }
  }
  exit(0);
}
