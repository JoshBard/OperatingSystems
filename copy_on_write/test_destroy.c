#include "tls.h"
#include <assert.h>

int main() {
  int check = tls_create(5000);
  assert(check == 0);
  check = tls_destroy();
  assert(check == 0);

  return 0;
}
