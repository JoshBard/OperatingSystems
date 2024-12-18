#include "tls.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

static void *test_inner_clone(void *tid){
  pthread_t temp = (pthread_t)tid;
  assert(tls_clone(temp) == 0);
  const char* original_data = "Original TLS data";
  char read_buffer[1024] = {0};
  assert(tls_read(0, strlen(original_data), read_buffer) == 0);
  assert(strcmp(read_buffer, original_data) == 0);

  const char* cloned_data = "Cloned TLS data";
  assert(tls_write(0, strlen(cloned_data), cloned_data) == 0);
  memset(read_buffer, 0, sizeof(read_buffer)); // Clear buffer    
  assert(tls_read(0, strlen(cloned_data), read_buffer) == 0);
  assert(strcmp(read_buffer, cloned_data) == 0);

  assert(tls_destroy() == 0);
  return 0;
}

void test_tls_clone_write_read() {
  assert(tls_create(1024) == 0); // Create a TLS with 1024 bytes

  const char* original_data = "Original TLS data";
  assert(tls_write(0, strlen(original_data), original_data) == 0);

  pthread_t tid = pthread_self(); // Get current thread ID
  pthread_t clone_tid;
  pthread_create(&clone_tid, NULL, &test_inner_clone, (void *)tid);
  pthread_join(clone_tid, NULL);

  char read_buffer[1024] = {0};
  assert(tls_read(0, strlen(original_data), read_buffer) == 0);
  assert(strcmp(read_buffer, original_data) == 0);

  assert(tls_destroy() == 0);
}

int main() {
  test_tls_clone_write_read();
  return 0;
}
