#include "TAG_Messaging.h"

int permAmmisible(int perm) {
  switch (perm) {
  case OPEN_ROOM:
  case PRIVATE_ROOM:
    return 0;
    break;
  default:
    return -1;
    break;
  }
}

int keyAmmissible(int key){
  if(key == 0){
    return -1;
  }
  return 0;
}

int validServiceOperation(TAG_Service* service) {
  if (current_cred()->uid.val == 0) // se è il super utente è SEMPRE valido
    return 1;

  switch (service->perm) {
  case OPEN_ROOM:
    return 1;
  case PRIVATE_ROOM:
    if (service->uid_Creator == current->tgid) // se è il mio processo è valido
      return 1;
    else
      return 0;
  default:
    return 0;
    break;
  }
}

TAG_Service* searchTAG_Service_withKey(TAG_Service **tagServices,int key){
  int i;
  for (i = 0; i < min_sys_TAG_Services ; i++){
    if(tagServices[i] != NULL){
      if (tagServices[i]->key == key){
          return tagServices[i];
      }
    }
  }
  return NULL;
}

TAG_Service* searchTAG_Service_withTag(TAG_Service **tagServices,int tag){
  int i;
  for (i = 0; i < min_sys_TAG_Services ; i++){
    if(tagServices[i] != NULL){
      if (tagServices[i]->tag == tag){
          return tagServices[i];
      }
    }
  }
  return NULL;
}

TAG_Service* new_TAG_Service(TAG_Service **tagServices,int key, int uid_Creator, int perm ){
  int i,j;
  for (i = 0; i < min_sys_TAG_Services ; i++){
    if(tagServices[i] == NULL){
      tagServices[i] = kmalloc(sizeof(TAG_Service), GFP_KERNEL | GFP_NOWAIT);
      tagServices[i]->key = key;
      tagServices[i]->tag = i;
      tagServices[i]->uid_Creator = uid_Creator;
      tagServices[i]->perm = perm;
      list_init(&tagServices[i]->levels);
      for (j = 0; j < levelDeep ; j++){
        // creazione dei 32 livelli per ogni TAG service.
        // ogni livello è implementato come un elemento di una lista RCU
        list_insert(&tagServices[i]->levels,j, "", 0);
      }
      AUDIT
      printk(KERN_INFO "TAG_Service Key: %d", tagServices[i]->key);
      AUDIT
      printk(KERN_INFO "TAG_Service Tag: %d", tagServices[i]->tag);
      AUDIT
      printk(KERN_INFO "TAG_Service creator ID: %d", tagServices[i]->uid_Creator);
      return tagServices[i];
    }
  }
  return NULL;
}

TAG_Service* getTAG_Service(TAG_Service **tagServices,int key, int uid_Creator, int perm ){
  int i;
  for (i = 0; i < min_sys_TAG_Services ; i++){
      if (tagServices[i]->key == 0){
            tagServices[i]->key = key;
            tagServices[i]->tag = i;
            tagServices[i]->uid_Creator = uid_Creator;
            tagServices[i]->perm = perm;
            AUDIT
            printk(KERN_INFO "Found service with tag ID %d", i);
            return tagServices[i];
          }
  }
  AUDIT
  printk(KERN_INFO "Returning NULL");
  return NULL;
}

char *scan_Service(size_t *len){
  int i;
  int level;
  size_t writeLen = 0;
  char *text = vzalloc(*len - 1);
  for (i = 0; i < min_sys_TAG_Services ; i++){
      if(tagServices[i] != NULL){
        for(level = 0; level < levelDeep ; level++){
          writeLen += scnprintf(text + writeLen,*len - writeLen, "Tag descriptor: %d;", tagServices[i]->tag);
          writeLen += scnprintf(text + writeLen,*len - writeLen, "Tag Creator: %d;", tagServices[i]->uid_Creator);
          writeLen += scnprintf(text + writeLen,*len - writeLen, "TAG ID: %d; KEY: %d; Level: %d, Waiters: %d", tagServices[i]->tag,tagServices[i]->key, level, tagServices[i]->levels.waiters[level]);
          writeLen += scnprintf(text + writeLen,*len - writeLen, "\n");
        }
        writeLen += scnprintf(text + writeLen,*len - writeLen, "\n");
      }
  }
  text[writeLen++] = '\0';
  *len = writeLen;
  return text;
}