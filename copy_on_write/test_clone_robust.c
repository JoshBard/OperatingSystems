#include "tls.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
static void *test_inner_clone(void *tid){
  pthread_t temp = (pthread_t)tid;
  assert(tls_clone(temp) == 0);
  char original_data[getpagesize()];
  for(int i = 0; i < getpagesize(); i++){
    original_data[i] = 'O';
  }
  original_data[getpagesize()] 
  
  char read_buffer[getpagesize()];
  assert(tls_read(0, getpagesize(), read_buffer) == 0);
  assert(strcmp(read_buffer, original_data) == 0);

  char cloned_data[getpagesize()];
  for(int i = 0; i < getpagesize(); i++){
    cloned_data[i] = 'C';
  }
  assert(tls_write(0, strlen(cloned_data), cloned_data) == 0);
  memset(read_buffer, 0, sizeof(read_buffer)); // Clear buffer
  assert(tls_read(0, getpagesize(), read_buffer) == 0);
  assert(strcmp(read_buffer, cloned_data) == 0);

  assert(tls_destroy() == 0);
  return 0;
}

void test_tls_clone_write_read() {
  assert(tls_create(getpagesize()) == 0); // Create a TLS with 1024 bytes

  char original_data[getpagesize()];
  for(int i = 0; i < getpagesize(); i++){
    original_data[i] = 'O';
  }
  assert(tls_write(0,getpagesize(), original_data) == 0);
  pthread_t tid = pthread_self(); // Get current thread ID
  pthread_t clone_tid;
  pthread_create(&clone_tid, NULL, &test_inner_clone, (void *)tid);
  pthread_join(clone_tid, NULL);

  char read_buffer[getpagesize()];
  assert(tls_read(0, getpagesize(), read_buffer) == 0);
  assert(strcmp(read_buffer, original_data) == 0);

  assert(tls_destroy() == 0);
}

int main() {
  test_tls_clone_write_read();
  return 0;
}
