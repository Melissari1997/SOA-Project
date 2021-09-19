#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <pthread.h>
#include <unistd.h>
#include <interface.h>

#define NUM_READER 50
#define NUM_WRITER 5

#define AUDIT if(1)
#define NService 5
#define PROBABILITY 5
int TAGS[NService];


void* receiver(void* id){
	int TAG_ID;
	int ret;
	int level;
	int size;
	int r;
	char receive_buffer[256];
	level = rand()%32;
	TAG_ID = rand()%NService;
	AUDIT
	printf("thread %ld - Waiting for receive on tag %d level %d\n",(long)id,TAGS[TAG_ID],level);
	ret = tag_receive(TAGS[TAG_ID], level, receive_buffer, sizeof(receive_buffer));
	AUDIT
	printf("thread %ld - Receive from tag %d on level %d: %s - Result %d\n",(long)id,TAGS[TAG_ID],level, receive_buffer, ret);
	return NULL;
}

void * sender(void* id){
	int TAG_ID;
	int ret;
	int level;
	int size;
	int r;
	int j;
  	char send_buffer[256];
	for (TAG_ID= 0; TAG_ID<NService ; TAG_ID++)	{
		for(level = 0; level < levelDeep; level++){
			r = rand()%100;
			if(r < PROBABILITY){
				int size = sprintf(send_buffer, "Message from thread %ld", (long)id);
				size++; // null caracter at end
				ret = tag_send(TAGS[TAG_ID], level, send_buffer, size);
				if(ret<0){
					AUDIT
					tagSend_perror(TAGS[TAG_ID]);
				}
			}
		}
	}	
	return NULL;
}

int main(int argc, char* argv){
    int keyAsk, commandAsk, permissionAsk, i, tag, j, THREADS;
	initTBDE();
	THREADS = NUM_READER + NUM_WRITER;
	pthread_t tid[THREADS];
    for (int i = 0; i < NService; i++){
        keyAsk = i + 1;
        commandAsk = CREATE;
        permissionAsk = OPEN_ROOM;
        tag = tag_get(keyAsk, commandAsk, permissionAsk);
        if (tag < 0) {
       	 tagGet_perror(keyAsk, commandAsk);
		 return -1;
        }
		AUDIT
		printf("Tag created with key: %d\n", i);
		TAGS[i] = tag;
    }

	for(i = 0; i < NUM_READER; i++){
		AUDIT
		printf("Receiver: %d \n", i);
		pthread_create(&(tid[i]),NULL,receiver,(void*)i);
	}

	for(i = 0; i < NUM_WRITER; i++){
		AUDIT
		printf("Sender: %d \n", NUM_READER + i);
		pthread_create(&(tid[NUM_READER + i]),NULL,sender,(void*)(NUM_READER+ i));
	}
	sleep(2);
	AUDIT
	printf("\n\nAwake All\n\n");
	int ret;
	
	for (j= 0; j<NService ; j++)	{
		ret = tag_ctl(j, AWAKE_ALL);
		if(ret != 0){
			tagCtl_perror(j,AWAKE_ALL);
		}
	}

	for(i=0; i<THREADS;i++){
		pthread_join(tid[i],NULL);
	}
	
	printf("\n\nStart deleting\n\n");
	for (j= 0; j<NService ; j++)	{
		ret = tag_ctl(TAGS[j], REMOVE);
		if(ret != 0){
			tagCtl_perror(j,REMOVE);
		}
		printf("Deleted service with TAG ID: %d\n" , TAGS[j]);
	}

	
	
}