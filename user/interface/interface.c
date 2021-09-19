#include "interface.h"

int sysCallIndex[4] = {0};

// int tag_get(int key, int command, int permission);
// int tag_send(int tag, int level, char *buffer, size_t size);
// int tag_receive(int tag, int level, char *buffer, size_t size);
// int tag_ctl(int tag, int command);

int convertStrtoArr(char *str, int numArr[], int limit) {
  int i = 0;
  char *token = strtok(str, ",");
  // loop through the string to extract all other tokens
  while (token != NULL && limit > 0) {
    numArr[i++] = atoi(token);
    token = strtok(NULL, ",");
    limit--;
  }
  return i;
}

void initTBDE() {
  char buf[64];
  FILE *fp;
  fp = fopen("/sys/module/TAG_Data_Messaging/parameters/sysCallNum", "r");
  if (!fp) {
    perror("fopen fail to read sysCallNum parameter");
    switch (errno) {
    case ENOENT:
      printf("Please, mound the module on the kernel:\n>>\tmake load\n");
      break;
    default:
      break;
    }
    exit(EXIT_FAILURE);
  }
  fgets(buf, sizeof(buf), fp);
  fclose(fp);
  int found = convertStrtoArr(buf, sysCallIndex, 4);
  for (int i = 0; i < found; i++) {
    printf("sysCallIndex[%d]=%d --> ", i, sysCallIndex[i]);
  }
  printf("END\n");
}

int tag_get(int key, int command, int permission) {
  if (sysCallIndex[0] != 0)
    return syscall(sysCallIndex[0], key, command, permission);
  else {
    printf("System not jet initialize!!\n Exiting...\n");
    exit(EXIT_FAILURE);
  }
}
int tag_send(int tag, int level, char *buffer, size_t size) {
  if (sysCallIndex[1] != 0)
    return syscall(sysCallIndex[1], tag, level, buffer, size);
  else {
    printf("System not jet initialize!!\n Exiting...\n");
    exit(EXIT_FAILURE);
  }
}
int tag_receive(int tag, int level, char *buffer, size_t size) {
  if (sysCallIndex[2] != 0)
    return syscall(sysCallIndex[2], tag, level, buffer, size);
  else {
    printf("System not jet initialize!!\n Exiting...\n");
    exit(EXIT_FAILURE);
  }
}
int tag_ctl(int tag, int command) {
  if (sysCallIndex[3] != 0)
    return syscall(sysCallIndex[3], tag, command);
  else {
    printf("System not jet initialize!!\n Exiting...\n");
    exit(EXIT_FAILURE);
  }
}

void tagGet_perror(int keyAsk, int commandAsk) {
  // @Return CREATE:
  //  succes            :=    tag descriptor
  //  ETOOMANYREFS      :=    numero di TAG Service oltre il limite
  //  EBADR             :=    Chiave già in uso
  //  EFAULT            :=    Errore generico
  // --
  // @Return OPEN:
  //  succes            :=    tag descriptor
  //  EBADSLT           :=    Chiave privata
  //  EBADRQC           :=    Permission invalid
  //  ENOMSG            :=    Chiave non trovata

  switch (errno) {
  case ENOSYS:
    printf("[tag_get] sysCall non più presente\n");
    return;
  case EBADRQC:
    printf("[tag_get] Parametro permission con valore non ammesso\n");
    return;
  case EILSEQ:
    printf("[tag_get] Comando non valido :%d\n", commandAsk);
    return;
  default:
    break;
  }

  if (commandAsk == CREATE) {
    switch (errno) {
    case ETOOMANYREFS:
      printf("[tag_get] CREATE key=%d, raggiunto il numero massimo di TAG_SERVICES\n", keyAsk);
      break;
    case EBADR:
      printf("[tag_get] CREATE key=%d, chiave già in uso\n", keyAsk);
      break;
    case EFAULT:
      printf("[tag_get] CREATE key=%d, Errore nella creazione del TAG_SERVICE\n", keyAsk);
    default:
      printf("[tag_get] CREATE , Unexpected error code return: %d\n", errno);
      break;
    }
    return;
  }

  if (commandAsk == OPEN) {
    switch (errno) {
    case EBADSLT:
      printf("[tag_get] OPEN @key=%d, la chiave si riferisce ad un TAG_Service privato\n", keyAsk);
      break;
    case EBADRQC:
      printf("[tag_get] OPEN @key=%d, permission non valida\n", keyAsk);
      break;
    case ENOMSG:
      printf("[tag_get] OPEN @key=%d, chiave non trovata\n", keyAsk);
      break;
    default:
      printf("[tag_get] OPEN , Unexpected error code return: %d\n", errno);
      break;
    }
    return;
  }

  printf("[tag_get] error, Unexpected error code return: %d\n", errno);
}

void tagSend_perror(int tag) {
  // @Return tag_send:
  //  succes            :=    return 0
  //  EXFULL            :=    Buffer troppo grande(>MAX_BUF_SIZE) o non specificato
  //  ENODATA           :=    Problema nella trasmissione al modulo kernel
  //  ENOMSG            :=    Tag non trovato
  //  EBADRQC           :=    Permission invalid
  //  EBADSLT           :=    Livello > levelDeep
  //  ENOSYS            :=    sysCall rimossa
  //  EILSEQ            :=    comando non valido
  //  EBADMSG           :=    Errore nella list-replace
  
  switch (errno) {
  case EXFULL:
    printf("[tag_send] @tag=%d, Buffer troppo grande(>MAX_BUF_SIZE) o non specificato\n", tag);
    break;
  case ENODATA:
    printf("[tag_send] @tag=%d, Problema nella trasmissione al modulo kernel\n", tag);
    break;
  case ENOMSG:
    printf("[tag_send] @tag=%d, Tag non trovato\n", tag);
    break;
  case EBADRQC:
    printf("[tag_send] @tag=%d, Permission invalid\n", tag);
    break;
  case EBADSLT:
    printf("[tag_send] @tag=%d, livello richiesto > livello massimo\n", tag);
    break;
  case ENOSYS:
    printf("[tag_send] sysCall non più presente\n");
    break;
  case EILSEQ:
    printf("[tag_send] @tag=%d, Command not valid\n", tag);
    break;
  case EBADMSG:
    printf("[tag_send] @tag=%d Errore nella list-replace\n", tag);
    break;
  default:
    printf("[tag_send] error, Unexpected error code return: %d\n", errno);
    break;
  }
}

void tagRecive_perror(int tag) {
  // @Return tag_receive:
  //  succes            :=    return size(message)
  //  EXFULL            :=    Buffer troppo grande (> MAX_BUF_SIZE), o non presente
  //  ENOMSG            :=    Tag non trovato
  //  EBADRQC           :=    Permessi non validi per eseguire l'operazione
  //  EBADSLT           :=    Livello > levelDeep
  //  ERESTART          :=    thread svegliato da un segnale
  //  EUCLEAN           :=    thread svegliato dal comando AWAKE_ALL
  //  ENOSYS            :=    sysCall rimossa
  //  EILSEQ            :=    comando non valido
  switch (errno) {
  case EXFULL:
    printf("[tag_receive] @tag=%d, Buffer troppo grande (> MAX_BUF_SIZE), o non presente\n", tag);
    break;
  case ENOMSG:
    printf("[tag_receive] @tag=%d, Tag non trovato\n", tag);
    break;
  case EBADRQC:
    printf("[tag_receive] @tag=%d, Permessi non validi per eseguire l'operazione\n", tag);
    break;
  case EBADSLT:
    printf("[tag_receive] @tag=%d, livello richiesto > livello massimo\n", tag);
    break;
  case ERESTART:
    printf("[tag_receive] @tag=%d, thread svegliato da un segnale\n", tag);
    break;
  case EUCLEAN:
    printf("[tag_receive] @tag=%d error, thread svegliato dal comando AWAKE_ALL\n", tag);
    break;
  case ENOSYS:
    printf("[tag_receive] sysCall non più presente\n");
    break;
  case EILSEQ:
    printf("[tag_receive] @tag=%d, comando non valido\n", tag);
    break;
  default:
    printf("[tag_receive] error, Unexpected error code return: %d\n", errno);
    break;
  }
}

void tagCtl_perror(int tag, int commandAsk) {
  // @Return AWAKE_ALL:
  //  succes            :=    return 0
  //  ENOMSG            :=    Nessun thread da svegliare
  //  EBADRQC           :=    Permessi non validi per eseguire l'operazione
  // @Return REMOVE:
  //  succes            :=    return 0
  //  ENODATA           :=    Il tag specificato non esiste
  //  EBADE             :=    Permessi non validi per eseguire l'operazione
  //  EADDRINUSE        :=    Reader in attesa in qualche livello
  if (errno == EILSEQ) {
    printf("[tag_ctl] error, comando non valido:%d\n", commandAsk);
    return;
  }

  switch (errno) {
  case ENOSYS:
    printf("[tag_ctl] sysCall removed\n");
    return;
  case ENOSR:
    printf("[tag_ctl] error, TAG ID < 0\n");
    return;
  case EILSEQ:
    printf("[tag_ctl] error, comando non valido:%d \n", commandAsk);
    return;
  default:
    break;
  }

  if (commandAsk == AWAKE_ALL) {
    switch (errno) {
    case ENOMSG:
      printf("[tag_ctl] TBDE_AWAKE_ALL @tag=%d, Niente da risvegliare\n", tag);
      break;
    case EBADRQC:
      printf("[tag_ctl] TBDE_AWAKE_ALL @tag=%d, Permessi non validi per eseguire l'operazione\n", tag);
      break;
    default:
      printf("[tag_ctl] TBDE_AWAKE_ALL, Unexpected error code return: %d\n", errno);
      break;
    }
    return;
  }

  if (commandAsk == REMOVE) {
    switch (errno) {
    case ENOSR:
      printf("[tag_ctl] TBDE_REMOVE @tag=%d,  TAG ID < 0\n", tag);
      break;
    case EBADE:
      printf("[tag_ctl] TBDE_REMOVE @tag=%d, Permessi non validi per eseguire l'operazione\n", tag);
      break;
    case EADDRINUSE:
      printf("[tag_ctl] TBDE_REMOVE @tag=%d, Reader in attesa in qualche livello\n", tag);
      break;
    case ENODATA:
      printf("[tag_ctl] TBDE_REMOVE @tag=%d, Niente da rimuovere\n", tag);
      break;
    default:
      printf("[tag_ctl] TBDE_REMOVE @tag=%d, Unexpected error code return: %d\n", tag, errno);
      break;
    }
    return;
  }

  printf("[tag_ctl] error, Unexpected error code return: %d\n", errno);
}
