#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <stddef.h>
#include <TAG_MACROS.h>
#include <linux/slab.h>
#include <linux/stddef.h>


#define  AUDIT if(0)

typedef struct _element{
	struct _element * next;
	int level;
	char* message;
	size_t size;
} element;


#ifndef LOCK

#ifndef PERIOD
#define PERIOD 1
#endif

typedef struct _rcu_list{
	int epoch;
	long standing[2];
	rwlock_t  write_lock;
	element * head;
	wait_queue_head_t readerQueue[levelDeep];
	int ready[levelDeep];
	int waiters[levelDeep];
} rcu_list;

typedef rcu_list list;

#define list_insert rcu_list_insert
#define list_search rcu_list_search
#define list_remove rcu_list_remove
#define list_init rcu_list_init
#define list_replace rcu_list_replace

//RCU versions
void rcu_list_init(rcu_list * l);

element* rcu_list_search(rcu_list *l, int key);

int rcu_list_insert(rcu_list *l, int key, char* msg, size_t size);

int rcu_list_remove(rcu_list *l, int key);

int rcu_list_replace(rcu_list *l, int  key, char* msg, size_t size);

#else

typedef struct _locked_list{
	pthread_spinlock_t lock;
	element * head;
} locked_list;

typedef locked_list list;

#define list_insert locked_list_insert
#define list_search locked_list_search
#define list_remove locked_list_remove
#define list_init locked_list_init

//lock-based versions

void locked_list_init(locked_list * l);

int locked_list_search (locked_list *l, long key);

int locked_list_insert(locked_list *l, long key);

int locked_list_remove(locked_list *l, long key);
#endif
