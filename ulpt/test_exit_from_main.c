#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <stdlib.h>
#include <assert.h>
#include "ec440threads.h"

void* testfunc(void *arg){
  int i = (long int) arg;
  printf("running test func %d\n",i);
  return NULL;
}

void pseudoschedule();

int main(){
  pthread_t temp;
  int arg = 2;

  pthread_create(&temp, NULL, (void *(*)(void *))testfunc, (void *)&arg);

  printf("thread created\n");
  printf("current id: %ld\n", pthread_self());
  printf("new thread ID: %ld\n", temp);
  pseudoschedule();
  
  int val = 1;
  int* addy = &val;

  printf("MADE IT HERE\n");
  pthread_exit(addy); 
  printf("SHOULD NOT MAKE IT HERE\n");
  return 0;
}
