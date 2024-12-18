#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <stdlib.h>
#include <assert.h>

void* testfunc(void *arg){
  int i = (long int) arg;
  printf("running test func %d\n",i);
  return NULL;
}

void print_queue();
void pseudoschedule();

int main(int argc, char **argv){
  void *ret;
  pthread_t newthread;
  unsigned long int i = 1;
  pthread_create(&newthread, NULL, testfunc, (void *)i);
  print_queue();
  printf("current id: %ld\n", pthread_self());
  printf("new thread ID: %ld\n", newthread);
  pseudoschedule();
  printf("current id after scheduling: %ld\n", pthread_self());
  print_queue();
  
  pthread_t secondthread;
  pthread_create(&secondthread, NULL, testfunc, (void*)i);
  printf("second thread ID: %ld\n", secondthread);
  pseudoschedule();
  printf("current id after scheduling: %ld\n", pthread_self());
  print_queue();
  
  pthread_t thirdthread;
  pthread_create(&thirdthread, NULL, testfunc, (void*)i);
  printf("third thread ID: %ld\n", thirdthread);
  print_queue();
  pseudoschedule();
  printf("current id after scheduling: %ld\n", pthread_self());
  print_queue();
  
  pthread_join(newthread, &ret);
  printf("thread exit status: %ld\n", *(unsigned long int*)ret);
  print_queue();
  
  return 0;
}
