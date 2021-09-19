#include "sysCall_Discovery.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Francesco Quaglia <francesco.quaglia@uniroma2.it>");
MODULE_DESCRIPTION("USCTM");

#define ADDRESS_MASK 0xfffffffffffff000 // to migrate

#define START                                                                                                          \
  0xffffffff00000000ULL // use this as starting address --> this is a biased
                        // search since does not start from 0xffff000000000000
#define MAX_ADDR 0xfffffffffff00000ULL
#define FIRST_NI_SYSCALL 134
#define SECOND_NI_SYSCALL 174
#define THIRD_NI_SYSCALL 182
#define FOURTH_NI_SYSCALL 183
#define FIFTH_NI_SYSCALL 214
#define SIXTH_NI_SYSCALL 215
#define SEVENTH_NI_SYSCALL 236
#define ENTRIES_TO_EXPLORE 256
#define PAGE_SIZE 4096
#define MAX_FREE 10

unsigned long *hacked_ni_syscall = NULL;
unsigned long **hacked_syscall_tbl = NULL;

unsigned long sys_call_table_address = 0x0;
unsigned long sys_ni_syscall_address = 0x0;

int free_entries[MAX_FREE];


int good_area(unsigned long *addr) {

  int i;

  for (i = 1; i < FIRST_NI_SYSCALL; i++) {
    if (addr[i] == addr[FIRST_NI_SYSCALL])
      goto bad_area;
  }

  return 1;

bad_area:

  return 0;
}

/* This routine checks if the page contains the begin of the syscall_table.  */
int validate_page(unsigned long *addr) {
  int i = 0;
  unsigned long page = (unsigned long)addr;
  unsigned long new_page = (unsigned long)addr;
  for (; i < PAGE_SIZE; i += sizeof(void *)) {
    new_page = page + i + SEVENTH_NI_SYSCALL * sizeof(void *);

    // If the table occupies 2 pages check if the second one is materialized in
    // a frame
    if (((page + PAGE_SIZE) == (new_page & ADDRESS_MASK)) && sys_vtpmo(new_page) == NO_MAP)
      break;
    // go for patter matching
    addr = (unsigned long *)(page + i);
    if (((addr[FIRST_NI_SYSCALL] & 0x3) == 0) && (addr[FIRST_NI_SYSCALL] != 0x0) // not points to 0x0
        && (addr[FIRST_NI_SYSCALL] > 0xffffffff00000000)                         // not points to a locatio lower than
                                                                                 // 0xffffffff00000000
        //&& ( (addr[FIRST_NI_SYSCALL] & START) == START )
        && (addr[FIRST_NI_SYSCALL] == addr[SECOND_NI_SYSCALL]) && (addr[FIRST_NI_SYSCALL] == addr[THIRD_NI_SYSCALL]) &&
        (addr[FIRST_NI_SYSCALL] == addr[FOURTH_NI_SYSCALL]) && (addr[FIRST_NI_SYSCALL] == addr[FIFTH_NI_SYSCALL]) &&
        (addr[FIRST_NI_SYSCALL] == addr[SIXTH_NI_SYSCALL]) && (addr[FIRST_NI_SYSCALL] == addr[SEVENTH_NI_SYSCALL]) &&
        (good_area(addr))) {
      hacked_ni_syscall = (void *)(addr[FIRST_NI_SYSCALL]); // save ni_syscall
      sys_ni_syscall_address = (unsigned long)hacked_ni_syscall;
      hacked_syscall_tbl = (void *)(addr); // save syscall_table address
      sys_call_table_address = (unsigned long)hacked_syscall_tbl;
      return 1;
    }
  }
  return 0;
}

/* 

  This routines looks for the syscall table.
  
 */
void syscall_table_finder(void) {
  unsigned long k;         // current page
  unsigned long candidate; // current page

  for (k = START; k < MAX_ADDR; k += 4096) {
    candidate = k;
    if ((sys_vtpmo(candidate) != NO_MAP)) {
      // check if candidate maintains the syscall_table
      if (validate_page((unsigned long *)(candidate))) {
        printk(KERN_INFO "syscall table found at %px\n", (void *)(hacked_syscall_tbl));
        printk(KERN_INFO "sys_ni_syscall found at %px\n", (void *)(hacked_ni_syscall));
        break;
      }
    }
  }
}

int foundFree_entries(int num) {
  int i, j;
  syscall_table_finder();

  if (!hacked_syscall_tbl) {
    printk( KERN_INFO "failed to find the sys_call_table\n");
    return -1;
  }

  j = 0;
  for (i = 0; i < ENTRIES_TO_EXPLORE; i++)
    if (hacked_syscall_tbl[i] == hacked_ni_syscall) {
      printk(KERN_INFO "found sys_ni_syscall entry at syscall_table[%d]\n", i);
      free_entries[j++] = i;
      if (j >= min(MAX_FREE, num))
        break;
    }
  return j;
}

unsigned long cr0;

static inline void write_cr0_forced(unsigned long val) {
  unsigned long __force_order;

  /* __asm__ __volatile__( */
  asm volatile("mov %0, %%cr0" : "+r"(val), "+m"(__force_order));
}

static inline void protect_memory(void) { write_cr0_forced(cr0); }

static inline void unprotect_memory(void) { write_cr0_forced(cr0 & ~X86_CR0_WP); }

int nextFree = 0;

int add_syscall(unsigned long sysPtr) {
  if (nextFree < MAX_FREE) {
    cr0 = read_cr0();
    unprotect_memory();
    hacked_syscall_tbl[free_entries[nextFree]] = (unsigned long *)sysPtr;
    protect_memory();
    printk(KERN_INFO "Sys_call has been installed as a trial on the sys_call_table at displacement %d\n",
                 free_entries[nextFree]);
    nextFree++;
    return free_entries[nextFree - 1];
  } else {
    printk(KERN_INFO "Impossible add sys_call on the the sys_call_table, ended the free entries\n");
    return -1;
  }
}

void removeAllSyscall(void) {
  printk(KERN_INFO "Cleaning of sysCall table modification...\n");
  cr0 = read_cr0();
  unprotect_memory();
  // restore delle entry della tabella
  for (; nextFree > 0; nextFree--) {
    printk(KERN_INFO "A sys_call [%d] are restore to ni_syscall\n", free_entries[nextFree - 1]);
    hacked_syscall_tbl[free_entries[nextFree - 1]] = (unsigned long *)hacked_ni_syscall;
  }

  protect_memory();
}
