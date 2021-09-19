#include "TAG_Messaging.h"

list key_list;
TAG_Service **tagServices;
rwlock_t searchLock;


unsigned int serviceCount;
int MAX_SERVICE_NUM;

int initService() {

  //predisposizione array di TAG_Services
  tagServices = kmalloc(sizeof(TAG_Service*) * min_sys_TAG_Services, GFP_KERNEL | GFP_NOWAIT);
  serviceCount = 0;
  rwlock_init(&searchLock);
  MAX_SERVICE_NUM = min_sys_TAG_Services;
  return 0;
}

void unmountRCU(){
  kfree(tagServices);
  tagServices = NULL;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// @Return CREATE:
//  succes            :=    tag descriptor
//  ETOOMANYREFS      :=    numero di TAG Service oltre il limite
//  EBADR             :=    Chiave già in uso
//  EFAULT            :=    Errore generico
int tag_get_CREATE(int key, int command, int permission) {
  TAG_Service *service;
  unsigned long flags;

  if (__sync_add_and_fetch(&serviceCount, 0) >= MAX_SERVICE_NUM) {
    AUDIT
    printk(KERN_ERR "[tag_get_CREATE] Impossible Create another room");
    module_put(THIS_MODULE);
    return -ETOOMANYREFS;
  }

  if(key == TAG_IPC_PRIVATE ){ // chiave privata
    write_lock_irqsave(&searchLock, flags);
    //creazione del nuovo service
    service = new_TAG_Service(tagServices, key, current->tgid, permission);
    if(service == NULL){
      module_put(THIS_MODULE);
      return -EBADR;
    }
    write_unlock_irqrestore(&searchLock, flags);

    refcount_set(&service->refCount, 1); 
    __sync_add_and_fetch(&serviceCount, 1); 

  } else{  //chiave pubblica
    write_lock_irqsave(&searchLock, flags);
    if(searchTAG_Service_withKey(tagServices, key) != NULL){
       write_unlock_irqrestore(&searchLock, flags);
       module_put(THIS_MODULE);
       return -EBADR;
    }
    //creazione del nuovo service
    service = new_TAG_Service(tagServices, key, current->tgid, permission);
    if(service == NULL){
      module_put(THIS_MODULE);
      return -EFAULT;
    }
    refcount_set(&service->refCount, 1); 
    AUDIT
    printk(KERN_INFO "[tag_get_CREATE] Service key: %d", key);
    __sync_add_and_fetch(&serviceCount, 1); 
    write_unlock_irqrestore(&searchLock, flags);
  }
  AUDIT
  printk(KERN_INFO "[tag_get_CREATE] New room created. Number of room: %d",__sync_add_and_fetch(&serviceCount, 0));
  module_put(THIS_MODULE);
  return service->tag;
}

// @Return OPEN:
//  succes            :=    tag descriptor
//  EBADSLT           :=    Chiave privata
//  EBADRQC           :=    Permission invalid
//  ENOMSG            :=    Chiave non trovata
int tag_get_OPEN(int key, int command, int permission) {
  TAG_Service *service;
  unsigned long flags;

  if (key == TAG_IPC_PRIVATE) {
    AUDIT
    printk(KERN_DEBUG "[tag_get_OPEN] Impossible to execute, the asked key is IPC_PRIVATE");
    return -EBADSLT;
  }
  read_lock_irqsave(&searchLock, flags);
  //cerco il servizio
  service = searchTAG_Service_withKey(tagServices, key);
  if(service){
    refcount_inc(&service->refCount);  
    read_unlock_irqrestore(&searchLock, flags);

    if (!validServiceOperation(service)) {
      module_put(THIS_MODULE);
      return -EBADRQC;
    }
    return service->tag;
  }
  read_unlock_irqrestore(&searchLock, flags);
  AUDIT
  printk(KERN_INFO "[tag_get_OPEN] No key are found");
  module_put(THIS_MODULE);
  return -ENOMSG;
}

// @Return CREATE:
//  succes            :=    tag descriptor
//  ETOOMANYREFS      :=    numero di TAG Service oltre il limite
//  EBADR             :=    Chiave già in uso
//  EFAULT            :=    Errore generico
// 
// Return OPEN:
//  succes            :=    tag descriptor
//  ETOOMANYREFS      :=    numero di TAG Service oltre il limite
//  EBADR             :=    Chiave già in uso
//  EFAULT            :=    Errore generico

/*

int tag_get(int key, int command, int permission);

*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 17, 0)
__SYSCALL_DEFINEx(3, _tag_get, int, key, int, command, int, permission) {
#else
asmlinkage long tag_get(int key, int command, int permission) {
#endif
  int ret;
  if (!try_module_get(THIS_MODULE)){
    return -ENOSYS;
  }

  if (permAmmisible(permission)) {
    AUDIT
    printk(KERN_DEBUG "[tag_get] Permission passed are Invalid");
    module_put(THIS_MODULE);
    return -EBADRQC;
  }
  
  if (keyAmmissible(key)){
    AUDIT
    printk(KERN_DEBUG "[tag_get] key passed is Invalid");
    module_put(THIS_MODULE);
    return -EILSEQ;
  }

  switch (command) {
  case CREATE:
    ret = tag_get_CREATE(key, command, permission);
    break;
  case OPEN:
    ret = tag_get_OPEN(key, command, permission);
    break;
  default:
    AUDIT
    printk(KERN_DEBUG "[tag_get] Invalid Command");
    ret = -EILSEQ;
    break;
  }
  return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 17, 0)
unsigned long tag_get = (unsigned long)__x64_sys_tag_get;
#else
#endif

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
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

// int tag_send(int tag, int level, char *buffer, size_t size);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 17, 0)
__SYSCALL_DEFINEx(4, _tag_send, int, tag, int, level, char *, buffer, size_t, size) {
#else
asmlinkage long tag_send(int tag, int level, char *buffer, size_t size) {
#endif
  TAG_Service *service;
  unsigned long flags;
  char *buf;

  if (!try_module_get(THIS_MODULE))
    return -ENOSYS;

  if (size > MAX_MESSAGE_SIZE || buffer == NULL) {
    module_put(THIS_MODULE);
    return -EXFULL;
  }

  //buffer che conterrà il messaggio da inviare
  buf = vzalloc(size);

  if (copy_from_user(buf, buffer, size) != 0) {
    AUDIT
    printk(KERN_DEBUG "[tag_send] impossible copy buffer from sender");
    module_put(THIS_MODULE);
    return -ENODATA;
  }

  read_lock_irqsave(&searchLock, flags);
  //cerco il servizio
  service = searchTAG_Service_withTag(tagServices, tag);
  if (!service) {
    read_unlock_irqrestore(&searchLock, flags);
    module_put(THIS_MODULE);
    return -ENOMSG;
  }
  read_unlock_irqrestore(&searchLock, flags);
  if (!validServiceOperation(service)) {
    vfree(buf);
    module_put(THIS_MODULE);
    return -EBADRQC;
  }

  if (level >= levelDeep) {
    vfree(buf);
    module_put(THIS_MODULE);
    return -EBADSLT;
  }
  int res;
  // faccio la replace sull'elemento della lista RCU che gestisce il livello specificato
  res = list_replace(&service->levels, level, buf, size);
  if(res != 1){
    AUDIT
    printk(KERN_DEBUG "[tag_send] Errore nella replace");
    module_put(THIS_MODULE);
    return -EBADMSG;
  }
  // controllo se ci sono dei thread attivi in ascolto
  int has_sleepers = waitqueue_active(&service->levels.readerQueue[level]);
  if(!has_sleepers){
    //non ci sono thread in ascolto, scarto il messaggio
    AUDIT
    printk(KERN_INFO "[tag_send] No waiters in queue");
    module_put(THIS_MODULE);
    return 0;
  }
  AUDIT
  printk(KERN_INFO "[tag_send] Wake_upping readers..." );
  // ci sono thread in sleep, li risveglio
  service->levels.ready[level] = 1;
  wake_up_all(&service->levels.readerQueue[level]);
  module_put(THIS_MODULE);
  return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 17, 0)
unsigned long tag_send = (unsigned long)__x64_sys_tag_send;
#else
#endif

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
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

/*

int tag_receive(int tag, int level, char *buffer, size_t size);

*/

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 17, 0)
__SYSCALL_DEFINEx(4, _tag_receive, int, tag, int, level, char *, buffer, size_t, size) {
#else
asmlinkage long tag_receive(int tag, int level, char *buffer, size_t size) {
#endif
  TAG_Service *service;
  element* e;
  
  unsigned long flags;
  size_t bSize, not_send, offset = 0;

  if (!try_module_get(THIS_MODULE))
    return -ENOSYS;

  if (size > MAX_MESSAGE_SIZE || buffer == NULL) {
    module_put(THIS_MODULE);
    return -EXFULL;
  }
  read_lock_irqsave(&searchLock, flags);
  // cerco il servizio col tag richiesto
  service = searchTAG_Service_withTag(tagServices, tag);
  if (!service) {
    read_unlock_irqrestore(&searchLock, flags);
    module_put(THIS_MODULE);
    return -ENOMSG;
  }
  read_unlock_irqrestore(&searchLock, flags);

  if (!validServiceOperation(service)) {
    module_put(THIS_MODULE);
    return -EBADRQC;
  }
  if (level >= levelDeep) {
    module_put(THIS_MODULE);
    return -EBADSLT;
  }
  AUDIT
  printk(KERN_INFO "[tag_receive] Listening for an RCU element ...");
  AUDIT
  printk(KERN_INFO "[tag_receive] Waiting for data ..."); 
  // mi metto in sleep sul livello richiesto
  e = list_search(&service->levels, level);
  if(e == NULL){
      module_put(THIS_MODULE);
      return -EUCLEAN;
  }
  bSize = min(size, e->size);
  //invio il risultato allo user. Controllo che tutti i byte siano stati inviati                                                                      
  while (bSize - offset > 0) {                                                                                       
    not_send = copy_to_user(buffer + offset, e->message + offset, bSize - offset);                                              
    offset += (bSize - offset) - not_send;                                                                             
  }    
  AUDIT
  printk(KERN_INFO "[tag_receive] Return data copied");
  module_put(THIS_MODULE);
  return bSize;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 17, 0)
unsigned long tag_receive = (unsigned long)__x64_sys_tag_receive;
#else
#endif

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// @Return AWAKE_ALL:
//  succes            :=    return 0
//  ENOMSG            :=    Nessun thread da svegliare
//  EBADRQC           :=    Permessi non validi per eseguire l'operazione
int tag_ctl_TBDE_AWAKE_ALL(int tag, int command) {
  TAG_Service*  service;
  unsigned long flags;
  int i;

  read_lock_irqsave(&searchLock, flags);
  service = searchTAG_Service_withTag(tagServices, tag);
  if (!service) {
    read_unlock_irqrestore(&searchLock, flags);
    module_put(THIS_MODULE);
    return -ENOMSG;
  }
  read_unlock_irqrestore(&searchLock, flags);

  if (!validServiceOperation(service)) {
    module_put(THIS_MODULE);
    return -EBADRQC;
  }
  for (i = 0; i < levelDeep; i++) {
    service->levels.ready[i] = 1;
    wake_up_all(&service->levels.readerQueue[i]);
  }
  AUDIT
  printk(KERN_INFO "[tag_ctl_AWAKE_ALL] All rooms wake_upped");
  write_unlock_irqrestore(&searchLock, flags);
  module_put(THIS_MODULE);
  return 0;
}

// @Return REMOVE:
//  succes            :=    return 0
//  ENODATA           :=    Il tag specificato non esiste
//  EBADE             :=    Permessi non validi per eseguire l'operazione
//  EADDRINUSE        :=    Reader in attesa in qualche livello
int tag_ctl_TBDE_REMOVE(int tag, int command) {
  TAG_Service* service;
  unsigned long flags;

  read_lock_irqsave(&searchLock, flags);
  service = searchTAG_Service_withTag(tagServices, tag);
  if (!service) {
    read_unlock_irqrestore(&searchLock, flags);
    module_put(THIS_MODULE);
    return -ENODATA;
  }
  if (!validServiceOperation(service)) {
    module_put(THIS_MODULE);
    return -EBADRQC;
  }
  int level;
  for(level = 0 ; level < levelDeep ; level++){
    int has_sleepers = waitqueue_active(&service->levels.readerQueue[level]);
    if(has_sleepers){
      AUDIT
      printk(KERN_DEBUG "[tag_ctl_TBDE_REMOVE] Tag to delete had some waiters reader");
      return -EADDRINUSE;
    }
  }
  AUDIT
  printk(KERN_INFO "[tag_ctl_TBDE_REMOVE] room with tag %d ", service->tag );
  TAG_Service* emptyService = kmalloc(sizeof(TAG_Service), GFP_KERNEL | GFP_NOWAIT);
  emptyService->key = 0;
  emptyService->tag = -1;
  list_init(&emptyService->levels);
  for (level = 0; level < levelDeep ; level++){
    // creazione dei 32 livelli per ogni TAG service.
    // ogni livello è implementato come un elemento di una lista RCU
    list_insert(&emptyService->levels,level, "", 0);
  }
  TAG_Service* tmp;
  tmp = tagServices[service->tag];
  tagServices[service->tag] = NULL;
  emptyService = tmp;
  kfree(emptyService);
  __sync_sub_and_fetch(&serviceCount, 1);
  read_unlock_irqrestore(&searchLock, flags); // libero il lock
  AUDIT
  printk(KERN_INFO "[tag_ctl_TBDE_REMOVE] Room (%d) are now Deleted, remaning %d rooms", tag,
            __sync_add_and_fetch(&serviceCount, 0));
  module_put(THIS_MODULE);
  return 0;
}

// @Return AWAKE_ALL:
//  succes            :=    return 0
//  ENOMSG            :=    Nessun thread da svegliare
//  EBADRQC           :=    Permessi non validi per eseguire l'operazione
// @Return REMOVE:
//  succes            :=    return 0
//  ENODATA           :=    Il tag specificato non esiste
//  EBADE             :=    Permessi non validi per eseguire l'operazione
//  EADDRINUSE        :=    Reader in attesa in qualche livello
/*

int tag_ctl(int tag, int command);

*/

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 17, 0)
__SYSCALL_DEFINEx(2, _tag_ctl, int, tag, int, command) {
#else
asmlinkage long tag_ctl(int tag, int command) {
#endif
  int ret;
  if (!try_module_get(THIS_MODULE)){
    module_put(THIS_MODULE);
    return -ENOSYS;
  }
  if (tag < 0) {
    module_put(THIS_MODULE);
    return -ENOSR;
  }

  switch (command) {
  case AWAKE_ALL:
    ret = tag_ctl_TBDE_AWAKE_ALL(tag, command);
    break;
  case REMOVE:
    ret = tag_ctl_TBDE_REMOVE(tag, command);
    break;
  default:
    ret = -EILSEQ;
    break;
  }
  return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 17, 0)
unsigned long tag_ctl = (unsigned long)__x64_sys_tag_ctl;
#else
#endif
