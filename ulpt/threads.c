#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <setjmp.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include "ec440threads.h"

#define MAX_THREADS 128			/* number of threads you support */
#define THREAD_STACK_SIZE (1<<15)	/* size of stack in bytes */
#define QUANTUM (50 * 1000)		/* quantum in usec */

enum thread_status
{
 TS_EXITED,
 TS_RUNNING,
 TS_READY
};

enum lock_status
{
 UNLOCKED,
 LOCKED
};

struct my_mutex{
  enum lock_status status;
  pthread_t ID;
};

struct mutex_queue{
  struct mutex_queue* next;
  struct my_mutex* current;
};

struct my_barrier{
  enum lock_status status;
  pthread_t ID;
  int count;
};

struct barrier_queue{
  struct barrier_queue* next;
  struct my_barrier* current;
};

struct thread_control_block {
  jmp_buf registers;
  pthread_t ID;
  enum thread_status status;
  void* exit_status;
  void* stack_ptr;
};

struct run_queue{
  struct thread_control_block* TCB;
  struct run_queue* next;
};

struct mutex_queue* FIRST_MUTEX;
struct mutex_queue* CURRENT_MUTEX;
struct mutex_queue* LAST_MUTEX;

struct run_queue* FIRST;
struct run_queue* CURRENT;
struct run_queue* LAST;

struct sigaction preempter;
struct sigaction atomic;

int count;

static void schedule(int signal)
{
  //check if the current is running, do not want to set exited as ready
  if(CURRENT -> TCB -> status == TS_RUNNING){
    CURRENT -> TCB -> status = TS_READY;
  }

  //saving current process and going to running on the next
  int err = setjmp(CURRENT -> TCB -> registers);
  if(err == 0){
    do{
      if(CURRENT -> next != NULL){
	CURRENT = CURRENT -> next;
      }else{
	CURRENT = FIRST;
      }
    }while(CURRENT -> TCB -> status == TS_EXITED);
    CURRENT -> TCB -> status = TS_RUNNING;
    longjmp(CURRENT -> TCB -> registers, 1);
  }
}

// to suppress compiler error saying these static functions may not be used...
static void schedule(int signal) __attribute__((unused));

static void scheduler_init()
{
  //creating the tcb, registers, and stack for main
  struct thread_control_block* tcb = (struct thread_control_block*)malloc(sizeof(struct thread_control_block));
  if(tcb <= 0){
    perror("Error: instantiating thread_control_block ");
  }
  tcb -> ID = (unsigned long int)tcb;

  void* stack = (void*)malloc(THREAD_STACK_SIZE);
  tcb -> stack_ptr = stack;
  stack += THREAD_STACK_SIZE - sizeof(void*);
  if(stack <= 0){
    perror("Error: instantiating stack_pointer ");
  }
  *(unsigned long int*)stack = (unsigned long int)pthread_exit;

  int err = setjmp(tcb -> registers);
  if(err < 0){
    perror("Error: setjmp failed ");
  }

  //creating the runqueue for main
  tcb -> status = TS_RUNNING;
  FIRST = (struct run_queue*)malloc(sizeof(struct run_queue));
  FIRST -> TCB = tcb;
  FIRST -> next = NULL;
  CURRENT = FIRST;
  LAST = FIRST;
	
  //alarm and signal handler
  ualarm(QUANTUM, QUANTUM);

  sigemptyset(&preempter.sa_mask);
  preempter.sa_handler = &schedule;
  preempter.sa_flags = SA_NODEFER;
  sigaction(SIGALRM, &preempter, 0);

  sigemptyset(&atomic.sa_mask);
  atomic.sa_handler = SIG_IGN;
  atomic.sa_flags = 0;
}

int pthread_create(
	pthread_t *thread, const pthread_attr_t *attr,
	void *(*start_routine) (void *), void *arg)
{
  //call init 
  static bool is_first_call = true;
  if (is_first_call) {
    is_first_call = false;
    scheduler_init();
  }
   
  //creating TCB
  struct thread_control_block* tcb = (struct thread_control_block*)malloc(sizeof(struct thread_control_block));
  if(tcb <= 0){
    perror("Error: instantiating thread_control_block ");
  }
  tcb -> ID = (unsigned long int)tcb;
  
  //mallocing stack to store
  void* stack = (void*)malloc(THREAD_STACK_SIZE);
  tcb -> stack_ptr = stack;
  stack += THREAD_STACK_SIZE - sizeof(void*);
  if(stack <= 0){
    perror("Error: instantiating stack_pointer ");
  }
  *(unsigned long int*)stack = (unsigned long int)pthread_exit;
  
  //creating the register state and adjusting the stored values
  int err = setjmp(tcb -> registers);
  if(err < 0){
    perror("Error: setjmp failed ");
  }

  //set r12 and 13 first, for start_thunk() to begin the call with
  set_reg(&tcb -> registers, JBL_R12, (unsigned long int)start_routine);
  set_reg(&tcb -> registers, JBL_R13, (unsigned long int)arg);
  set_reg(&tcb -> registers, JBL_RSP, (unsigned long int)stack);
  set_reg(&tcb -> registers, JBL_PC, (unsigned long int)start_thunk);
  tcb -> status = TS_READY;
 
  //adding to the runqueue
  struct run_queue* curr_rq = (struct run_queue*)malloc(sizeof(struct run_queue));
  curr_rq -> TCB = tcb;
  curr_rq -> next = NULL;
  
  LAST -> next = curr_rq;
  LAST = curr_rq;

  *thread = LAST -> TCB -> ID;
  return 0;
}

void pthread_exit(void *value_ptr)
{
  //check if current process is first/main
  if(CURRENT == FIRST){
    FIRST = CURRENT -> next;
    printf("HELLO\n");
    CURRENT -> TCB -> status = TS_EXITED;
    CURRENT -> TCB -> exit_status = value_ptr;
    exit(0);
  }
	
  //regular behavior 
  CURRENT -> TCB -> status = TS_EXITED;
  CURRENT -> TCB -> exit_status = value_ptr;
  schedule(SIGALRM);
  exit(1);
}

pthread_t pthread_self(void)
{
  //return the running ID
  return CURRENT -> TCB -> ID;
}

int pthread_join(pthread_t thread, void **retval)
{
  //create a previous pointer that we use to connect to next
  struct run_queue* curr_rq = FIRST;
  struct run_queue* prev_rq = curr_rq;
  while(curr_rq -> TCB -> ID != thread){
    prev_rq = curr_rq;
    curr_rq = curr_rq -> next;
  }
  //dont want to join/free everything once the process has actually exited
  while(curr_rq -> TCB -> status != TS_EXITED){
    schedule(SIGALRM);
  }
  //update the runqueue and assign retval if it is not null
  if(curr_rq -> TCB -> status == TS_EXITED){
    prev_rq -> next = curr_rq -> next;
    if(thread == LAST -> TCB -> ID){
      LAST = prev_rq;
    }
    if(retval != NULL){
      *retval = curr_rq->TCB->exit_status;
    }
    free(curr_rq -> TCB -> stack_ptr);
    free(curr_rq -> TCB);
  }
  return 0;
}

int is_initialized = 0;

int pthread_mutex_init(pthread_mutex_t *restrict mutex, const pthread_mutexattr_t *restrict attr){
  struct my_mutex* new = (struct my_mutex*)mutex;
  new -> status = LOCKED;
  new -> ID = pthread_self();
  if(!is_initialized){
    FIRST_MUTEX -> current = new;
    LAST_MUTEX -> current = new;
    LAST_MUTEX -> next = NULL;
    CURRENT_MUTEX = FIRST_MUTEX;
    is_initialized = 1;
  }else{
    struct mutex_queue* temp_mutex = (struct mutex_queue*)malloc(sizeof(struct mutex_queue));
    temp_mutex -> current = new;
    LAST_MUTEX -> next = temp_mutex;
    LAST_MUTEX = temp_mutex;
    LAST_MUTEX -> next = NULL;
  }
  return 0;
}

int pthread_mutex_destroy(pthread_mutex_t *mutex){
  struct mutex_queue* current_mutex = FIRST_MUTEX;
  struct mutex_queue* previous_mutex = current_mutex;
  pthread_t deleted_mutex = ((struct my_mutex*)mutex) -> ID;
  while(current_mutex -> next != NULL){
    if(current_mutex -> current -> ID == deleted_mutex){
      break;
    }
    previous_mutex = current_mutex;
    current_mutex = current_mutex -> next;
  }

  if(current_mutex == LAST_MUTEX){
    previous_mutex -> next = NULL;
    LAST_MUTEX = previous_mutex;
  }
  if(current_mutex == FIRST_MUTEX){
    if(current_mutex -> next != NULL){
      FIRST_MUTEX = current_mutex -> next;
    }else{
      FIRST_MUTEX = previous_mutex;
    }
  }
  if(current_mutex != LAST_MUTEX && current_mutex != FIRST_MUTEX){
    previous_mutex -> next = current_mutex -> next;
  }
  free(current_mutex);
  return 0;
}

int pthread_mutex_lock(pthread_mutex_t *mutex){
  //turn off sig alarm
  sigaction(SIGALRM, &atomic, 0);
  ((struct my_mutex*)mutex) -> status = LOCKED;
  return 0;
}

int pthread_mutex_unlock(pthread_mutex_t *mutex){
  //turn on sig alarm
  sigaction(SIGALRM, &preempter, 0);
  *mutex = MUTEX_UNLOCKED;
  return 0;
}

int pthread_barrier_init(pthread_barrier_t *restrict barrier, const pthread_barrierattr_t *restrict attr, unsigned count){
  return 0;
}

int pthread_barrier_destroy(pthread_barrier_t *barrier){
  return 0;
}

int pthread_barrier_wait(pthread_barrier_t *barrier){
  return 0;
}



