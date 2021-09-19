
#include <stdio.h>
#include <stdlib.h>
#include <interface.h>

void help() {
  printf("usage: <tag_id> <cmd_id>\n");
  printf("\tREMOVE = %d\n", REMOVE);
  printf("\tAWAKE_ALL = %d\n", AWAKE_ALL);
}

int tag;
int commandAsk;

int main(int argc, char **argv) {
  char buf[64];
  if (argc != 3) {
    help();
    exit(-1);
  }
  initTBDE();

  tag = atoi(argv[1]);
  commandAsk = atoi(argv[2]);

  printf("tag_ctls(%d, %d)\n", tag, commandAsk);
  if (tag_ctl(tag, commandAsk) == -1) {
    tagCtl_perror(tag, commandAsk);
  }
}
