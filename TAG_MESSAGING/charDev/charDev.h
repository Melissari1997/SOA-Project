#ifndef charDev_H
#define charDev_H

#define EXPORT_SYMTAB
#include "../TAG_Subsystem/TAG_Messaging.h"
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pid.h> /* For pid types */
#include <linux/sched.h>
#include <linux/tty.h>     /* For the tty declarations */
#include <linux/version.h> /* For LINUX_VERSION_CODE */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0)
#define get_major(session) MAJOR(session->f_inode->i_rdev)
#define get_minor(session) MINOR(session->f_inode->i_rdev)
#else
#define get_major(session) MAJOR(session->f_dentry->d_inode->i_rdev)
#define get_minor(session) MINOR(session->f_dentry->d_inode->i_rdev)
#endif

#define DEVICE_NAME "TAG_MESSAGING" /* Device file name in /dev/ - not mandatory  */

typedef struct _scanModuleStatus {
  char *message;
  ssize_t size;
} scanModuleStatus;

#define STATUS_SIZE_MAX (20*4096)

scanModuleStatus *getModuleStatus(void);
void freeInstance(scanModuleStatus *inst);

/* the actual driver */
int dev_open(struct inode *, struct file *);
int dev_release(struct inode *, struct file *);
ssize_t dev_read(struct file *filp, char *buff, size_t len, loff_t *off);
loff_t dev_llseek(struct file *filp, loff_t f_pos, int whence);

extern struct file_operations fileOps;
extern int majorNum;
extern dev_t devNo;          // Major and Minor device numbers combined into 32 bits
extern struct class *class; // class_create will set this

int devkoInit(void);
void devkoExit(void);

#endif