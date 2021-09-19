#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <stddef.h>
#include <linux/preempt.h>
#include <linux/syscalls.h>
#include <linux/sysfs.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/vmalloc.h>
#include <linux/wait.h>
#include "list.h"

unsigned long flags;


void rcu_list_init(rcu_list * l){
	l->epoch = 0;
	l->standing[0] = 0;
	l->standing[1] = 0;
	l->head = NULL;
	rwlock_init(&l->write_lock);
	
	int i;
	for(i = 0; i < levelDeep; i++){
		l->ready[i] = 0;
		l->waiters[i] = 0;
		init_waitqueue_head(&l->readerQueue[i]);
	}

}

element* rcu_list_search(rcu_list *l, int key){
	int retWait;
	int my_epoch = l->epoch;
	asm volatile("mfence");
	//preempt_disable();
	l->ready[key] = 0; 
	l->waiters[key] +=1; 
	retWait = wait_event_interruptible(l->readerQueue[key], __sync_add_and_fetch(&l->ready[key], 0) == 1);
	if (retWait == -ERESTARTSYS) {
		__sync_fetch_and_add(&l->standing[my_epoch],-1);
		l->waiters[key] -=1;
		return NULL;
	}  
	__sync_fetch_and_add(&l->standing[my_epoch],1);

	element *p = l->head;
	//actual list traversal		
	while(p!=NULL){
		if ( p->level == key){
			break;
		}
		p = p->next;
	}
	__sync_fetch_and_add(&l->standing[my_epoch],-1);
	l->waiters[key] -=1;
	//preempt_enable();
	if (p){
		return p;
	} 
	if(p == NULL){
		return NULL;
	}
	return NULL;

}

int rcu_list_insert(rcu_list *l, int key, char* msg, size_t size){

	element * p;
	int temp;
	int grace_epoch;
	
	p = kmalloc(sizeof(element),GFP_KERNEL);

	if(!p) return 0;

	p->level = key;
	p->next = NULL;
	p->message = msg;
	p->size = size;
	//preempt_disable();
	write_lock_irqsave(&(l->write_lock), flags);

	//traverse and insert
	p->next = l->head;
	l->head = p;


	//goto done;
	
	//move to a new epoch - still under write lock
	grace_epoch = temp = l->epoch;
	temp +=1;
	temp = temp%2;
	l->epoch = temp;
	asm volatile("mfence");
	//preempt_enable();
	
	while(l->standing[grace_epoch] > 0);
	

	p = l->head;
	

done:
	write_unlock_irqrestore(&l->write_lock, flags);
	
	return 1;

}

int rcu_list_remove(rcu_list *l, int key){

	element * p, *removed = NULL;
	int temp;
	int grace_epoch;
	
	write_lock_irqsave(&l->write_lock, flags);

	//traverse and delete
	p = l->head;
	if(p != NULL && p->level == key){
		removed = p;
		l->head = removed->next;
	}
	else{
		while(p != NULL){
			if ( p->next != NULL && p->next->level == key){
				removed = p->next;
				p->next = p->next->next;
				break;
			}
			p = p->next;	
		}
	}
	//move to a new epoch - still under write lock
	grace_epoch = temp = l->epoch;
	temp +=1;
	temp = temp%2;
	while(l->standing[temp] > 0);
	l->epoch = temp;
	asm volatile("mfence");

	while(l->standing[grace_epoch] > 0);
	
	write_unlock_irqrestore(&l->write_lock, flags);

	if(removed){
		kfree(removed);
		return 1;
	}
	return 0;
}

int rcu_list_replace(rcu_list *l, int  key, char* msg, size_t size){
	element * elem, *new_element, *removed = NULL;
	int temp;
	int grace_epoch;


	new_element = kmalloc(sizeof(element),GFP_KERNEL);
	if(!new_element) return 0;

	new_element->level = key;
	new_element->next = NULL;
	new_element->message = msg;
	new_element->size = size;
	//preempt_disable();
	write_lock_irqsave(&l->write_lock, flags);

	//	Attraverso la lista
	elem = l->head;
	if(elem == NULL)
		return 0;
	if(elem->level == key){
		removed = elem;
		new_element->next = removed->next;
		//inserisco il nuovo elemento al posto di quello rimosso
		l->head = new_element;
	}
	else{
		while(elem != NULL){
			if ( elem->next != NULL && elem->next->level == key){
				removed = elem->next;
				//inserisco il nuovo elemento al posto di quello rimosso
				elem->next = new_element;
				new_element->next = removed->next;
				break;
			}
			elem = elem->next;	
		}
	}
	if(elem == NULL){
		return 0;
	}
	grace_epoch = temp = l->epoch;
	temp +=1;
	temp = temp%2;
	
	//	Nuova epoca.
	l->epoch = temp;
	//preempt_enable();
	asm volatile("mfence");

	//	Aspetto tutti quelli che, mentre veniva eseguita la replace,
	//	non avevano ancora finito la rcu_search
	while(l->standing[grace_epoch] > 0);
	write_unlock_irqrestore(&l->write_lock, flags);

	if(removed){
		kfree(removed);
		return 1;
	}
	return 0;
}