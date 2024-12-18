#include "tls.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>

#define MAX_THREAD_COUNT 128

/*
 * This is a good place to define any data structures you will use in this file.
 * For example:
 *  - struct TLS: may indicate information about a thread's local storage
 *    (which thread, how much storage, where is the storage in memory)
 *  - struct page: May indicate a shareable unit of memory (we specified in
 *    homework prompt that you don't need to offer fine-grain cloning and CoW,
 *    and that page granularity is sufficient). Relevant information for sharing
 *    could be: where is the shared page's data, and how many threads are sharing it
 *  - Some kind of data structure to help find a TLS, searching by thread ID.
 *    E.g., a list of thread IDs and their related TLS structs, or a hash table.
 */

/* add sigaction and siginfo*/
typedef struct thread_local_storage {
  pthread_t tid;
  unsigned int size;      /* size in bytes                    */
  unsigned int page_num;  /* number of pages                  */
  struct page **pages;    /* array of pointers to pages       */
} TLS;

struct page {
  size_t address;   /* start address of page            */
  int ref_count;          /* counter for shared pages         */
};

/*
 * Now that data structures are defined, here's a good place to declare any
 * global variables.
 */

struct tid_tls_pair{
  pthread_t tid;
  TLS *tls;
  int used;
};

static struct tid_tls_pair tid_tls_pairs[MAX_THREAD_COUNT] = {0};

/*
 * With global data declared, this is a good point to start defining your
 * static helper functions.
 */

/*
 * Lastly, here is a good place to add your externally-callable functions.
 */
void tls_protect(struct page *p){
  if(mprotect((void *) p->address, getpagesize(), PROT_NONE)){
    fprintf(stderr, "tls_protect: could not protect page\n");
    exit(1);
  }
}

void tls_unprotect(struct page *p){
  if(mprotect((void *) p->address, getpagesize(), PROT_WRITE)){
    fprintf(stderr, "tls_unprotect: could not unprotect page\n");
    exit(1);
  }
}

void tls_handle_page_fault(int sig, siginfo_t *si, void *context){
  // printf("I GOT CALLED\n");
  unsigned int p_fault = ((unsigned long int) si->si_addr) & ~(getpagesize() - 1);
  for(int i = 0; i < MAX_THREAD_COUNT; i++){
    for(int j = 0; j < tid_tls_pairs[i].tls -> page_num; j++){
      if((tid_tls_pairs[i].tls == NULL) | (tid_tls_pairs[i].tls -> pages == NULL)){
	continue;
      }
      if((unsigned int)tid_tls_pairs[i].tls -> pages[j] -> address == p_fault){
	pthread_exit(NULL);
      }
    }
  }
  signal(SIGSEGV, SIG_DFL);
  signal(SIGBUS, SIG_DFL);
  raise(sig);
}

void tls_init(){
  struct sigaction sigact;
  //int page_size = getpagesize();
  /* Handle page faults (SIGSEGV, SIGBUS) */
  sigemptyset(&sigact.sa_mask); /* Give context to handler */
  sigact.sa_flags = SA_SIGINFO;
  sigact.sa_sigaction = tls_handle_page_fault;
  sigaction(SIGBUS, &sigact, NULL);
  sigaction(SIGSEGV, &sigact, NULL);
}

int initialized = 0;

int tls_create(unsigned int size)
{
  if(!initialized){
    tls_init();
    initialized = 1;
  }

  if(size == 0){
    return -1;
  }
  
  int first_index = -1;
  pthread_t curr_pid = pthread_self();
  for(int i = 0; i < MAX_THREAD_COUNT; i++){
    if(tid_tls_pairs[i].tid == curr_pid){
      return -1;
    }
  }
  for(int i = 0; i < MAX_THREAD_COUNT; i++){
    if(!tid_tls_pairs[i].used){
      first_index = i;
      break;
    }
  }
  if(first_index == -1){
    return -1;
  }
  
  TLS *ptr = (TLS*)malloc(sizeof(TLS));
  if(ptr == NULL){
    perror("TLS malloc Error");
    exit(1);
  }
  tid_tls_pairs[first_index].tls = ptr;
  tid_tls_pairs[first_index].tls -> size = size;
  tid_tls_pairs[first_index].tls -> tid = curr_pid;

  int page_num = size/getpagesize();
  if(size%getpagesize() != 0){
    page_num += 1;
  }
  tid_tls_pairs[first_index].tls -> page_num = page_num;

  struct page **page_array = (struct page**)malloc(sizeof(struct page *) * page_num);

  for(int i = 0; i < page_num; i++){
    struct page *page_ptr = (struct page *) malloc(sizeof(struct page));
    if(page_ptr == NULL){
      perror("Page malloc Error");
      exit(1);
    }
    page_array[i] = page_ptr;
    page_array[i] -> address = (long unsigned int)mmap(0, getpagesize(), PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
    if((void*)page_array[i] -> address == MAP_FAILED){
      perror("mmap unsuccesful");
      exit(1);
    }
    page_array[i] -> ref_count = 0;
  }
  tid_tls_pairs[first_index].tls -> pages = page_array;

  tid_tls_pairs[first_index].tid = curr_pid;
  tid_tls_pairs[first_index].used = 1;
  return 0;
}

int tls_destroy()
{
  int curr_index = -1;
  pthread_t curr_pid = pthread_self();
  for(int i = 0; i < MAX_THREAD_COUNT; i++){
    if(tid_tls_pairs[i].tid == curr_pid){
      curr_index = i;
      break;
    }
  }
  if(curr_index == -1){
    return -1;
  }
  for(int i = 0; i < tid_tls_pairs[curr_index].tls -> page_num; i++){
    if(tid_tls_pairs[curr_index].tls -> pages[i] -> ref_count == 0){
      munmap((void*)tid_tls_pairs[curr_index].tls -> pages[i] -> address, getpagesize());
      free(tid_tls_pairs[curr_index].tls -> pages[i]);
    }else{
      tid_tls_pairs[curr_index].tls -> pages[i] -> ref_count--;
    }
  }
  free(tid_tls_pairs[curr_index].tls -> pages);
  free(tid_tls_pairs[curr_index].tls);
  tid_tls_pairs[curr_index].used = 0;
  tid_tls_pairs[curr_index].tid = -1;
  return 0;
}

int tls_read(unsigned int offset, unsigned int length, char *buffer)
{
  int curr_index = -1;
  pthread_t curr_pid = pthread_self();
  for(int i = 0; i < MAX_THREAD_COUNT; i++){
    if(tid_tls_pairs[i].tid == curr_pid){
      curr_index = i;
      break;
    }
  }

  if(curr_index == -1){
    return -1;
  }

  TLS* current = tid_tls_pairs[curr_index].tls;

  int start_page = offset/getpagesize();
  int offset_rem = offset%getpagesize();
  if(current -> size < offset + length){
    return -1;
  }

  int i = offset_rem;
  tls_unprotect(current -> pages[start_page]);
  for(int j = 0; j < length; j++){
    buffer[j] = *(char*)(current -> pages[start_page] -> address + i);
    i++;
    if(i == getpagesize()){
      i = 0;
      tls_protect(current -> pages[start_page]);
      start_page++;
      if(start_page == current -> page_num){
	break;
      }
      tls_unprotect(current -> pages[start_page]);
    }
  }
  if(start_page != current -> page_num){
    tls_protect(current -> pages[start_page]);
  }
  return 0;
}

int tls_write(unsigned int offset, unsigned int length, const char *buffer)
{
  int curr_index = -1;
  pthread_t curr_pid = pthread_self();
  for(int i = 0; i < MAX_THREAD_COUNT; i++){
    if(tid_tls_pairs[i].tid == curr_pid){
      curr_index = i;
      break;
    }
  }
  if(curr_index == -1){
    return -1;
  }
  TLS* current = tid_tls_pairs[curr_index].tls;

  int start_page = offset/getpagesize();
  int offset_rem = offset%getpagesize();
  if(current -> size < offset + length){
    return -1;
  }

  if(current -> pages[start_page] -> ref_count > 0){
    tls_unprotect(current -> pages[start_page]);
    current -> pages[start_page] -> ref_count--;
    struct page* copy = (struct page *) malloc(sizeof(struct page));
    copy -> address = (unsigned long int)mmap(0, getpagesize(), PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
    for(int k = 0; k < getpagesize(); k++){
      *(char*)(copy -> address + k) = *(char*)(current -> pages[start_page] -> address + k);
    }
    tls_protect(current -> pages[start_page]);
    copy -> ref_count = 0;
    current -> pages[start_page] = copy;
  }
  tls_unprotect(current -> pages[start_page]);
  int i = offset_rem;

  for(int j = 0; j < length; j++){
    *(char*)(current -> pages[start_page] -> address + i) = buffer[j];
    i++;
    
    if(i == getpagesize()){
      tls_protect(current -> pages[start_page]);
      i = 0;
      start_page++;
      if(start_page == current -> page_num){
	break;
      }
      tls_unprotect(current -> pages[start_page]);
      
      if(current -> pages[start_page] -> ref_count > 0){
      	current -> pages[start_page] -> ref_count--;
	struct page* copy = (struct page *) malloc(sizeof(struct page));
	copy -> address = (unsigned long int)mmap(0, getpagesize(), PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
	for(int k = 0; k < getpagesize(); k++){
	  *(char*)(copy -> address + k) = *(char*)(current -> pages[start_page] -> address + k);
	}
	tls_protect(current -> pages[start_page]);
	current -> pages[start_page] = copy;
	current -> pages[start_page] -> ref_count = 0;
	tls_unprotect(current -> pages[start_page]);
      }
    }
  }
  if(start_page != current -> page_num){
    tls_protect(current -> pages[start_page]);
  }
  return 0;
}

int tls_clone(pthread_t tid)
{
  int curr_index = -1;
  pthread_t curr_pid = pthread_self();
  for(int i = 0; i < MAX_THREAD_COUNT; i++){
    if(tid_tls_pairs[i].tid == curr_pid){
      return -1;
      break;
    }
  }
  for(int i = 0; i < MAX_THREAD_COUNT; i++){
    if(!tid_tls_pairs[i].used){
      curr_index = i;
      break;
    }
  }
  if(curr_index == -1){
    return -1;
  }

  int target_index = -1;
  for(int i = 0; i < MAX_THREAD_COUNT; i++){
    if(tid_tls_pairs[i].tid == tid){
      target_index = i;
      break;
    }
  }
  if(target_index == -1){
    return -1;
  }
  TLS *ptr = (TLS*)malloc(sizeof(TLS));
  if(ptr == NULL){
    perror("TLS malloc Error");
    exit(1);
  }
  int page_num = tid_tls_pairs[target_index].tls -> page_num;
  tid_tls_pairs[curr_index].tls = ptr;
  tid_tls_pairs[curr_index].tls -> page_num = tid_tls_pairs[target_index].tls -> page_num;
  tid_tls_pairs[curr_index].tls -> size = tid_tls_pairs[target_index].tls -> size;

  struct page **page_array = (struct page**)malloc(sizeof(struct page *) * page_num);

  for(int i = 0; i < page_num; i++){
    tid_tls_pairs[target_index].tls -> pages[i] -> ref_count += 1;
    page_array[i] = tid_tls_pairs[target_index].tls -> pages[i];
  }

  tid_tls_pairs[curr_index].tls -> pages = page_array;
  tid_tls_pairs[curr_index].tid = pthread_self();
  tid_tls_pairs[curr_index].used = 1;
   
  return 0;
}
