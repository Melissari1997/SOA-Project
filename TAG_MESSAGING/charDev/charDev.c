#include "charDev.h"

scanModuleStatus *getModuleStatus() {
  scanModuleStatus *status = kzalloc(sizeof(scanModuleStatus), GFP_USER); 
  status->size = STATUS_SIZE_MAX;                         // verrà ridimensionata dalla scan_Service
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
struct class *class; // class_create will set this

//callback che definisce che il device avrà nonme dev_name(dev) e sarà nella cartella /dev/MODNAME
static char *dev_devnode(struct device *dev, umode_t *mode) {
  if (mode != NULL)
    *mode = 0444;
  return kasprintf(GFP_KERNEL, "%s/%s", MODNAME, dev_name(dev));
  ;
}

int devkoInit(void) {
  struct device *device;

  // Registro il char device (creo un link tra il device ed il corrispondente /dev file nel kernel space)
  majorNum = register_chrdev(0, DEVICE_NAME, &fileOps);
  if (majorNum < 0) {
    printk(KERN_ERR "Could not register device: %d\n", majorNum);
    return majorNum;
  }
  devNo = MKDEV(majorNum, 0); // Create a dev_t, 32 bit version of numbers

  // Voglio renderlo accessibile in user space

  // Creo /sys/class/DEVICE_NAME per poi creare /dev/DEVICE_NAME --> virtual device, ritorna un puntatore a struct class
  // che verrà usato per chiamare la device_create

  class = class_create(THIS_MODULE, DEVICE_NAME);
  if (IS_ERR(class)){
    printk(KERN_ERR "can't create /sys/%s class", DEVICE_NAME);
    unregister_chrdev_region(devNo, 1);
    return -1;
  }
  //rende accessibile il driver su /dev
  class->devnode = dev_devnode;
  // Creo /dev/DEVICE_NAME, da cui sarà accessibile il Char Device
  if (IS_ERR(device = device_create(class, NULL, devNo, NULL, DEVICE_NAME))) {
    printk(KERN_ERR "can't create device /dev/%s/%s\n", MODNAME, DEVICE_NAME);
    class_destroy(class);
    unregister_chrdev_region(devNo, 1);
    return -1;
  }
  printk(KERN_INFO "%s: new device registered, it is assigned major number %d\n",MODNAME, majorNum);
  return 0;
}

void devkoExit(void) {
  unregister_chrdev(majorNum, DEVICE_NAME);
  // Clean up after ourselves
  device_destroy(class, devNo);            // Remove the /dev/kmem
  class_destroy(class);                    // Remove class /sys/class/kmem
  unregister_chrdev(majorNum, DEVICE_NAME); // Unregister the device

  printk(KERN_INFO "new device unregistered, it was assigned major number %d\n", majorNum);

  return;
} // end of devkoExit
