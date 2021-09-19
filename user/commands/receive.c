
#include <stdio.h>
#include <stdlib.h>
#include <interface.h>

void help() { printf("usage: <tag> <level>\n"); }

int tag, level;

int main(int argc, char **argv) {
  if (argc != 3) {
    help();
    exit(-1);
  }
  initTBDE();

  tag = atoi(argv[1]);
  level = atoi(argv[2]);
  char buf[4096];

  int bRead = tag_receive(tag, level, buf, sizeof(buf));
  printf("@bRead = %d = tag_receive(%d,%d,bufPtr, bufSize)\n", bRead, tag, level);
  if (bRead < 0)
    tagRecive_perror(tag);
  else {
    printf("[reader]> %s\tReturn value = %d\n", buf, bRead);
  }
}
