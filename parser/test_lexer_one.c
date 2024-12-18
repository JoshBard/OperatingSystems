#include "myshell_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int
main(void)
{
  char pre_string[] = "hello > goodbye";
  struct lexed_string* my_lexed = lexer(pre_string);
  // Check print statements
  printf("%s \n", my_lexed->predirect);
  printf("%s \n", my_lexed->redirect);
  
  // Test that the string was split correctly
  assert(strcmp(my_lexed->predirect,"hello ") == 0);
  assert(strcmp(my_lexed->redirect,"> goodbye") == 0);

  free(my_lexed);
}
