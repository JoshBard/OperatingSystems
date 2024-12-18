#include "myshell_parser.h"
#include "stddef.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

//stuff that is supposed to go in .h but gradescope was grumpy
struct lexed_string {
  char predirect[MAX_LINE_LENGTH]; // string before a > or <
  char redirect_in[MAX_LINE_LENGTH]; // file or function being redirected
  char redirect_out[MAX_LINE_LENGTH];
};

struct lexed_string* lexer(char* piped_temp);

void remove_characters(char* input);

struct pipeline *pipeline_build(const char *command_line)
{
  // TODO: Implement this function
    // struct pipeline_command Pipeline_Command;
    // delimiting on '|', must make a copy because strtok() changes the input string
    char command_line_copy[MAX_LINE_LENGTH];
    strcpy(command_line_copy, command_line);
    remove_characters(command_line_copy);
    char* temp_piped = NULL;
    char* rest = command_line_copy;
    temp_piped = strtok_r(command_line_copy, "|", &rest);

    //variables for within the loop
    struct lexed_string* Lexed_String;
    struct pipeline_command* Pipeline_Command = (struct pipeline_command*)malloc(sizeof(struct pipeline_command));
    char* predirect_string;
    char* temp_whitespace;
    char* redirect_string;
   
    //instantiating the pipeline struct, this just points to the first pipeline command, this also needs to be malloced
    struct pipeline* Pipeline = (struct pipeline*)malloc(sizeof(struct pipeline));
    Pipeline->commands = Pipeline_Command;
    if(strchr(command_line, '&') != NULL){
      Pipeline->is_background = 1;
    }else{
      Pipeline->is_background = 0;
    }
    
    int i = 0;
    while(temp_piped != NULL){
        Lexed_String = lexer(temp_piped);
        predirect_string = Lexed_String->predirect;
        i = 0;
        temp_whitespace = strtok(predirect_string, " ");
        while(temp_whitespace != NULL && i <257){
	  char* copy = (char*)malloc(MAX_LINE_LENGTH * sizeof(char));
     	  strcpy(copy, temp_whitespace);
	  remove_characters(copy);
	    Pipeline_Command->command_args[i] = copy;
            temp_whitespace = strtok(NULL, " ");
            i++;
        }
	Pipeline_Command -> command_args[i] = NULL;
        redirect_string = Lexed_String->redirect_out;
	if(redirect_string != NULL){
	    temp_whitespace = strtok(redirect_string, "> ");
	    if(temp_whitespace != NULL){
	    char* copy = (char*)malloc(MAX_LINE_LENGTH * sizeof(char));
      	    strcpy(copy, temp_whitespace);
   	    remove_characters(copy);
	    Pipeline_Command->redirect_out_path = copy;
	    }
	}
	redirect_string = Lexed_String->redirect_in;
	if(redirect_string != NULL){
	    temp_whitespace = strtok(redirect_string, "< ");
	    if(temp_whitespace != NULL){
	    char* copy = (char*)malloc(MAX_LINE_LENGTH * sizeof(char));
     	    strcpy(copy, temp_whitespace);
	    remove_characters(copy);
	    Pipeline_Command->redirect_in_path = copy;
	    }
	}
	free(Lexed_String);
	temp_piped = strtok_r(rest, "|", &rest);
	if(temp_piped != NULL){
	  Pipeline_Command -> next = (struct pipeline_command*)malloc(sizeof(struct pipeline_command));
	  Pipeline_Command = Pipeline_Command -> next;
	  Pipeline_Command -> next = NULL;
	}
    }
    Pipeline_Command -> next = NULL;
    return Pipeline;
}

void pipeline_free(struct pipeline *pipeline)
{
  // TODO: Implement this function
  struct pipeline_command* next;
  struct pipeline_command* Pipeline_Command;
  Pipeline_Command = pipeline -> commands;
  next = Pipeline_Command -> next;
  do{
    for(int i = 0; Pipeline_Command -> command_args[i] != NULL; i++){
      free(Pipeline_Command -> command_args[i]);
    }
    free(Pipeline_Command -> redirect_in_path);
    free(Pipeline_Command -> redirect_out_path);
    next = Pipeline_Command -> next;
    free(Pipeline_Command);
    Pipeline_Command = next;
  }while(next != NULL);
  free(pipeline);
}

struct lexed_string* lexer(char* piped_temp) {
    struct lexed_string *Lexed_String = (struct lexed_string *)malloc(sizeof(struct lexed_string));
    Lexed_String -> redirect_in[0] = '\0';
    Lexed_String -> redirect_out[0] = '\0';
    Lexed_String -> predirect[0] = '\0';
    
    int i = 0;
    int finish = strlen(piped_temp) - 1;
    while((piped_temp[i] != '>' && piped_temp[i] != '<') && finish >= i){
      i++;
    }
    strncpy(Lexed_String->predirect, piped_temp, i);
    Lexed_String->predirect[i] = '\0';

    int j = i;
    if(piped_temp[i] == '>'){
      while(piped_temp[j] != '<' && j <= finish){
	j++;
      }
      strncpy(Lexed_String -> redirect_out, piped_temp + i, j - i);
      Lexed_String -> redirect_out[j - i] = '\0';

      if(piped_temp[j] == '<'){
	strncpy(Lexed_String -> redirect_in, piped_temp + j, finish - j + 1);
	Lexed_String -> redirect_in[finish - j + 1] = '\0';
      }
    }

      int k = i;
      if(piped_temp[i] == '<'){
	while(piped_temp[k] != '>' && k <= finish){
	  k++;
	}
	strncpy(Lexed_String -> redirect_in, piped_temp + i, k - i);
	Lexed_String -> redirect_in[k - i] = '\0';

	if(piped_temp[k] == '>'){
	  strncpy(Lexed_String -> redirect_out, piped_temp + k, finish - k + 1);
	  Lexed_String -> redirect_out[finish - k + 1] = '\0';
	}
      }
    return(Lexed_String);
}

void remove_characters(char* input){
  int start = 0;
  while(isspace(input[start])){
      start++;
  }
  int finish = strlen(input) - 1;
  while((isspace(input[finish]) || input[finish] == '&') && finish >= start){
    finish--;
  }
  int i = 0;
  while(i + start <= finish){
    input[i] = input[i + start];
    i++;
  }
  input[i] = '\0';
}
