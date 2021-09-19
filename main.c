#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>

#include "TAG_MESSAGING/charDev/charDev.h"
#include "TAG_MESSAGING/sysCall_Discovery/sysCall_Discovery.h"
#include "TAG_MESSAGING/TAG_Subsystem/TAG_Messaging.h"

#define AUDIT if (0)

// This array expose the syscall number of the syscall function:
// [0] int tag_get(int key, int command, int permission);
// [1] int tag_send(int tag, int level, char *buffer, size_t size);
// [2] int tag_receive(int tag, int level, char *buffer, size_t size);
// [3] int tag_ctl(int tag, int command);
int sysCallNum[4];
module_param_array(sysCallNum, int, NULL, 0444); // only readable

#define STR_VALUE(arg) #arg
#define FUNCTION_NAME(name) STR_VALUE(name)

#define exposeNewSyscall(sysPtr, num)                                                                                  \
  do {                                                                                                                 \
    sysCallNum[num] = add_syscall(sysPtr);                                                                             \
    if (sysCallNum[num] == -1) {                                                                                       \
      printk(KERN_INFO "Module fail to mount at add_syscall(%s) \n", FUNCTION_NAME(sysPtr));                                \
      return -ENOMEM;                                                                                                  \
    } else                                                                                                             \
      printk(KERN_INFO "sysCallNum[%d]=%d [%s(...)]\n", num, sysCallNum[num], FUNCTION_NAME(sysPtr));                       \
  } while (0)

/* Routine to execute when loading the module. */
int init_module_Default(void) {
  int error;
  int freeFound;
  printk(KERN_INFO "Initializing\n");

  freeFound = foundFree_entries(4);
  printk(KERN_INFO "Found %d entries\n", freeFound);
  if (freeFound > 0) {
    error = initService();
    if (error)
      return error;
    exposeNewSyscall(tag_get, 0);
    exposeNewSyscall(tag_send, 1);
    exposeNewSyscall(tag_receive, 2);
    exposeNewSyscall(tag_ctl, 3);
  }
  printk(KERN_INFO "Module correctly mounted\n");
  devkoInit();
  return 0;
}

void cleanup_module_Default(void) {
  removeAllSyscall();
  devkoExit();
  unmountRCU();
  printk(KERN_INFO "Shutting down\n");
}

module_init(init_module_Default);
module_exit(cleanup_module_Default);

MODULE_AUTHOR("Paolo Melissari <melissaripaolo@outlook.it");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("TAG_Data_Messaging");
