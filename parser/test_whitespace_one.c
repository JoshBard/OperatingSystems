#include "myshell_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int
main(void)
{
  char string[] = " Hello     ";
  remove_characters(string);
  // Test that white spaces were removed properly
  assert(strcmp(string,"Hello") == 0);
  assert(strlen(string) == 5);
}
