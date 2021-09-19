#ifndef TAG_Messaging_h
#define TAG_Messaging_h

#include <stddef.h>

#include <TAG_MACROS.h>

#include "list.h"
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kobject.h>
#include <linux/param.h>
#include <linux/preempt.h>
#include <linux/syscalls.h>
#include <linux/sysfs.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/vmalloc.h>
#include <linux/wait.h>

#define min_sys_TAG_Services 256

#define  AUDIT if(0)
// Struttura che definisce il TAG Service

typedef struct TAG_Service{
  refcount_t refCount;
  int key;             // chiave del tag service
  unsigned int tag;    // valore del tag
  int uid_Creator;   // PID del creatore
  int perm;   // permessi
  list levels; // lista contenente i 32 livelli, ognuno implementato come lista RCU
} TAG_Service;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
extern TAG_Service **tagServices;
extern rwlock_t searchLock;
extern unsigned int serviceCount; // Safe increment thanks searchLock
extern int tagCounting;        // Safe increment thanks searchLock
extern int MAX_SERVICE_NUM;
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

int initService(void);     // shuld be call BEFORE installation of syscall
void unmountTBDE(void); // shuld be call AFTER installation of syscall
void unmountRCU(void);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 17, 0)
extern unsigned long tag_get;
extern unsigned long tag_send;
extern unsigned long tag_receive;
extern unsigned long tag_ctl;
#else
int tag_get(int key, int command, int permission);
int tag_send(int tag, int level, char *buffer, size_t size);
int tag_receive(int tag, int level, char *buffer, size_t size);
int tag_ctl(int tag, int command);
#endif

int permAmmisible(int perm);

int validServiceOperation(TAG_Service* service);
int keyAmmissible(int key);

TAG_Service *getTAG_Service(TAG_Service **tagServices,int key, int uid_Creator, int perm );
TAG_Service* searchTAG_Service_withKey(TAG_Service **tagServices,int key);
TAG_Service* searchTAG_Service_withTag(TAG_Service **tagServices,int tag);
TAG_Service* new_TAG_Service(TAG_Service **tagServices,int key, int uid_Creator, int perm );
char *scan_Service(size_t *len);

#endif