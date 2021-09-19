#include "charDev.h"

scanModuleStatus *getModuleStatus() {
  scanModuleStatus *status = kzalloc(sizeof(scanModuleStatus), GFP_USER); // todo: verificare che forse debba essere kernel space
  status->size = STATUS_SIZE_MAX;                         // verrà ridimensionata con la chiamata successiva
  status->message = scan_Service(&status->size);
  return status;
}

void freeInstance(scanModuleStatus *status) {
  vfree(status->message);
  kfree(status);
}

int dev_open(struct inode *inode, struct file *file) {
  printk(KERN_INFO "[dev_open]");
  file->private_data = getModuleStatus();
  return 0;
}

int dev_release(struct inode *inode, struct file *file) {
  printk(KERN_INFO "[dev_release]");
  freeInstance(file->private_data);
  return 0;
}

ssize_t dev_read(struct file *filp, char *buff, size_t len, loff_t *f_pos) {
  scanModuleStatus *dev = filp->private_data;
  size_t not_send, offset = 0;
  printk(KERN_INFO "[dev_read] @len=%ld  @dev->size=%ld", len, dev->size);
  printk(KERN_INFO "data read:\n%s", dev->message);

  if (*f_pos >= dev->size) // se sono oltre il file
    return 0;

  if (*f_pos + len > dev->size) // se la richiesta è superiore alla dimensione, ritornerò i byte mancanti
    len = dev->size - *f_pos;
  printk(KERN_INFO "data to copy =%ld", len);
  
  while (len - offset > 0) {                                                                                       
    not_send = copy_to_user(buff + offset, dev->message + *f_pos + offset, len - offset);                                              
    offset += (len - offset) - not_send;                                                                             
  }    
  *f_pos += len;
  return len;
}

struct file_operations fileOps = {
    .owner = THIS_MODULE,   // do not forget this
    .open = dev_open,       // Allocate in private_data the buffer to be read
    .release = dev_release, // De-Allocate in private_data the buffer readed
    .read = dev_read,       // Read the buffer
};

int majorNum;
dev_t devNo;          // Major and Minor device numbers combined into 32 bits
struct class *pClass; // class_create will set this

static char *dev_devnode(struct device *dev, umode_t *mode) {
  printk("\n\n****%s: %d\n\n", __func__, __LINE__);
  if (mode != NULL)
    *mode = 0444;
  return kasprintf(GFP_KERNEL, "%s/%s", MODNAME, dev_name(dev));
  ;
}

int devkoInit(void) {
  struct device *pDev;

  // Register character device
  majorNum = register_chrdev(0, DEVICE_NAME, &fileOps);
  if (majorNum < 0) {
    printk(KERN_ERR "Could not register device: %d\n", majorNum);
    return majorNum;
  }
  devNo = MKDEV(majorNum, 0); // Create a dev_t, 32 bit version of numbers

  // Create /sys/class/DEVICE_NAME in preparation of creating /dev/DEVICE_NAME

  pClass = class_create(THIS_MODULE, DEVICE_NAME);
  if (IS_ERR(pClass)){
    printk(KERN_ERR "can't create /sys/%s class", DEVICE_NAME);
    unregister_chrdev_region(devNo, 1);
    return -1;
  }
  pClass->devnode = dev_devnode;
  // Create /dev/DEVICE_NAME for this char dev
  if (IS_ERR(pDev = device_create(pClass, NULL, devNo, NULL, DEVICE_NAME))) {
    printk(KERN_ERR "can't create device /dev/%s/%s\n", MODNAME, DEVICE_NAME);
    class_destroy(pClass);
    unregister_chrdev_region(devNo, 1);
    return -1;
  }
  printk(KERN_INFO "%s: new device registered, it is assigned major number %d\n",MODNAME, majorNum);
  return 0;
} // end of devkoInit

void devkoExit(void) {
  unregister_chrdev(majorNum, DEVICE_NAME);
  // Clean up after ourselves
  device_destroy(pClass, devNo);            // Remove the /dev/kmem
  class_destroy(pClass);                    // Remove class /sys/class/kmem
  unregister_chrdev(majorNum, DEVICE_NAME); // Unregister the device

  printk(KERN_INFO "new device unregistered, it was assigned major number %d\n", majorNum);

  return;
} // end of devkoExit
