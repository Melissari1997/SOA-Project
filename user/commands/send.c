
#include <stdio.h>
#include <stdlib.h>
#include <interface.h>

void help() { printf("usage: <tag> <level> <msg>\n"); }

int tag, level;

int main(int argc, char **argv) {
  if (argc != 4) {
    help();
    exit(-1);
  }
  initTBDE();

  tag = atoi(argv[1]);
  level = atoi(argv[2]);

  int size = strlen(argv[3]);
  size++; // last null caracter
  printf("tag_send(%d,%d,%p,%d)\n", tag, level, argv[3], size);
  if (tag_send(tag, level, argv[3], size) == -1)
    tagSend_perror(tag);
}
