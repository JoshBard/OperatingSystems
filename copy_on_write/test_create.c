#include "tls.h"
#include <assert.h>

int main() {
  int check = tls_create(3);
  assert(check == 0);
  check = tls_create(3);
  assert(check == -1);
  
  return 0;
}
