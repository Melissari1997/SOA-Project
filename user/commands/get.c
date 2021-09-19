
//#include "tbdeUser/tbdeUser.h"
#include <stdio.h>
#include <stdlib.h>
#include <interface.h>

void help() {
  printf("usage: <key> <command> <permission>\n");
  printf("command:\n");
  printf("\tCREATE = %d\n", CREATE);
  printf("\tOPEN = %d\n", OPEN);
  printf("permission:\n");
  printf("\tPUBLIC_ROOM = %d\n", OPEN_ROOM);
  printf("\tPRIVATE_ROOM = %d\n", PRIVATE_ROOM);
}

int tag;
int keyAsk, commandAsk, permissionAsk;

int main(int argc, char **argv) {
  char buf[64];
  if (argc != 4) {
    help();
    exit(-1);
  }
  initTBDE();

  keyAsk = atoi(argv[1]);
  commandAsk = atoi(argv[2]);
  permissionAsk = atoi(argv[3]);

  tag = tag_get(keyAsk, commandAsk, permissionAsk);
  printf("@tag = %d = tag_get(%d,%d,%d)\n", tag, keyAsk, commandAsk, permissionAsk);
  if (tag == -1) {
    printf("Error");
    tagGet_perror(keyAsk, commandAsk);
  }
}
