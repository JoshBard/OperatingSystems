#include "tls.h"
#include <assert.h>
#include <signal.h>


int main(){
  tls_init();
  /*
  struct sigaction sigact;
  //int page_size = getpagesize();
  sigemptyset(&sigact.sa_mask); 
  sigact.sa_flags = SA_SIGINFO;
  sigact.sa_sigaction = &handler;
  sigaction(SIGBUS, &sigact, NULL);
  sigaction(SIGSEGV, &sigact, NULL);
  */
  raise(SIGSEGV);
  assert(1);
  return 0;
}
