#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <stdlib.h>
#include <assert.h>

void* testfunc(void *arg){
  int i = (long int) arg;
  printf("running test func %d\n",i);
  return 0;
}

void pseudoschedule();

int main(int argc, char **argv){
  pthread_t newthread;
  unsigned long int i = 1;
  pthread_create(&newthread, NULL, testfunc, (void *)i);
  printf("thread created\n");
  printf("current id: %ld\n", pthread_self());
  printf("new thread ID: %ld\n", newthread);
  pseudoschedule();
  printf("current id after scheduling: %ld\n", pthread_self());
  pseudoschedule();
  printf("current id after scheduling: %ld\n", pthread_self());
  pseudoschedule();
  printf("current id after scheduling: %ld\n", pthread_self());
  pseudoschedule();
  printf("current id after scheduling: %ld\n", pthread_self());
  pseudoschedule();
  printf("current id after scheduling: %ld\n", pthread_self());
  pthread_t secondthread;
  pthread_create(&secondthread, NULL, testfunc, (void*)i);
  pseudoschedule();
  printf("current id after scheduling: %ld\n", pthread_self());
  pseudoschedule();
  printf("current id after scheduling: %ld\n", pthread_self());
  pseudoschedule();
  printf("current id after scheduling: %ld\n", pthread_self());
  pseudoschedule();
  printf("current id after scheduling: %ld\n", pthread_self());

  return 0;
}
