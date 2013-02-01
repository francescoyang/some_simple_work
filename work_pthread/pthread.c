
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
pthread_mutex_t mutex;
pthread_cond_t  c1cond;
pthread_cond_t  c2cond;
int times = 0;
void * child1(void *arg)
{
//	pthread_cleanup_push(pthread_mutex_unlock,&mutex);  /* comment 1 */
	printf("thread 1 get running \n");
	while(1){
		pthread_mutex_lock(&mutex);
		pthread_cond_wait(&c1cond,&mutex);
		
		printf("\n");
		printf("thread 1 condition applied  thread1 times is %d\n",times++);
		printf("\n");
		pthread_mutex_unlock(&mutex);
      	//sleep(2);
    }
//        pthread_cleanup_pop(0);     /* comment 2 */
}
void *child2(void *arg)
{
    printf("thread 2 get running.\n");
	while(1){
		pthread_mutex_lock(&mutex);
		pthread_cond_wait(&c2cond,&mutex);

		printf("\n");
		printf("thread 2 condition applied  thread2 times is %d\n",times++);
		printf("\n");
		pthread_mutex_unlock(&mutex);
		//sleep(2);
	}
}

int main(void)
{
	pthread_t tid1,tid2;
	printf("hello, condition variable test\n");
	pthread_mutex_init(&mutex,NULL);
	pthread_cond_init(&c1cond,NULL);
	pthread_cond_init(&c2cond,NULL);
	pthread_create(&tid1,NULL,child1,NULL);
	pthread_create(&tid2,NULL,child2,NULL);

	while(1){
		pthread_cond_signal(&c1cond);
		        sleep(2);                   /* comment 6 */
		pthread_cond_signal(&c2cond);
    }
    pthread_exit(0);
}
