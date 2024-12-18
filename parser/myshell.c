#include "myshell_parser.h"
#include "myshell.h"
#include "stddef.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include "fcntl.h"

//function headers
void execute_shell(int, char**);
void foreground_handler(int);
void background_handler(int);

//main file that runs the execute shell function, passes into execute_shell the argc and argv functions that are need to do the -n prompt suppressor.
int
main(int argc, char* argv[]){
  execute_shell(argc, argv);
  return 0;
}

//main shell function
void execute_shell(int argc, char* argv[]){
  //this is where I declare the variables that are "global" to all processes. For example, the array pipefd must be the same for all children so they are accessing the same pipes.
  //some of these variables are "local" to a process, but they should not be instantiated repeatedly, as this will throw errors.
  char command_line[MAX_LINE_LENGTH];
  int pipefd[32][2];
  pid_t cpid;
  int status;
  int process_total;
  int current_process;
  struct pipeline* built_pipeline;
  struct sigaction foreground_action, background_action;

  //checks if my_shell should be suppressed.
  if(!(argc > 1 && strcmp(argv[1], "-n") == 0)){
    printf("my_shell$ ");
  }

  //main shell loop that executes until told not to.
  while(1){
    // reading input from user
    if(fgets(command_line, MAX_LINE_LENGTH, stdin) == NULL){
      exit(0);
    }

    //parsing
    built_pipeline = pipeline_build(command_line);

    //my parser removes all whitespace, so if the command is just enter this checks for null and reprompts the user
    while(built_pipeline -> commands -> command_args[0] == NULL){
      printf("my_shell$ ");
      // this is the check for CTRL-D which is the EOF marker and so fgets is NULL as it is trying to "get" the end of a file
      if(fgets(command_line, MAX_LINE_LENGTH, stdin) == NULL){
	exit(0);
      }
      pipeline_free(built_pipeline);
      built_pipeline = pipeline_build(command_line);
    }
    
    //additional check for "exit"
    if(strcmp(built_pipeline -> commands -> command_args[0], "exit") == 0){
      exit(0);
    }

    //incrementing to create a total_process count that is used many times as the upper bound for loops throughout the shell
    process_total = 1;
    struct pipeline_command *next_process = built_pipeline -> commands -> next;
    while(next_process != NULL){
      process_total += 1;
      next_process = next_process -> next;
    }

    // loop that forks each process. A child will leave this loop with current_process, which tells it which process it is as well as with a pipe if there are pipes. The index for pipes is current_process - 1 as current_process begins at 1.
    current_process = 0;
    for(int i = 0; i < process_total; i++){
      if(i != process_total - 1){
	pipe(pipefd[i]);
      }
      cpid = fork();
      if(cpid == 0){
	current_process = i + 1;
	i = process_total;
      }
    }
       
    //child process, cpid = 0
    if(cpid == 0){
      // this loops creates the commands for the current child.
      struct pipeline_command* current_command;
      next_process = built_pipeline -> commands;
      for(int j = 0; j < current_process; j++){
	current_command = next_process;
	next_process = current_command -> next;
      }
      char *in = current_command -> redirect_in_path;
      char *out = current_command -> redirect_out_path;

      // redirect logic, just opening, closing, and dup2ing like a boss. Open uses the O_CREAT to files that do not exist and O_TRUNC to empty files. 
      if(in != NULL){
	int file_descriptor = open(in, O_RDONLY);
	if(file_descriptor < 0){
	  perror("ERROR: opening file ");
	  exit(EXIT_FAILURE);
	}
	close(0);
	dup2(file_descriptor, 0);
	close(file_descriptor);
      }
      if(out != NULL){
	int file_descriptor = open(out, O_WRONLY | O_CREAT | O_TRUNC);
	if(file_descriptor < 0){
	  perror("ERROR: opening file ");
	  exit(EXIT_FAILURE);
	}
	close(1);
	dup2(file_descriptor, 1);
	close(file_descriptor);
      }

      //changing pipe outputs, same closing and dup2ing logic as the redirects but now we need to close all ends of the pipes that a process can use.
      //implemented in a way that Larry suggested, using first, middle, and last children.
      //first child always writes
      if(current_process == 1 && process_total > 1){
	close(1);
	dup2(pipefd[current_process - 1][1],1);
	close(pipefd[current_process - 1][1]);
	close(pipefd[current_process - 1][0]);
      }
      //middle child always reads and writes
      else if(current_process > 1 && current_process < process_total){
	close(0);
	close(1);
	dup2(pipefd[current_process - 2][0],0);
	dup2(pipefd[current_process - 1][1],1);
	close(pipefd[current_process - 2][0]);
	close(pipefd[current_process - 2][1]);
	close(pipefd[current_process - 1][0]);
	close(pipefd[current_process - 1][1]);
	//additional loop to close all the pipes that were used by this process, kept getting infinite waitpids so just went extra safe on closing pipes, same thing in last child as well
	for(int j = 0; (current_process - 3 >= 0 && j <= current_process - 3); j++){
	  close(pipefd[j][1]);
	  close(pipefd[j][0]);
	}
      }
      //last child always reads
      else if(process_total > 1 && current_process == process_total){
	close(0);
	dup2(pipefd[current_process - 2][0],0);
	close(pipefd[current_process - 2][0]);
	close(pipefd[current_process - 2][1]);
	for(int j = 0; (current_process - 3 >= 0 && j <= current_process - 3); j++){
	  close(pipefd[j][1]);
	  close(pipefd[j][0]);
	}
      }

      //executing arguments and successful exit message
      char** arguments = current_command->command_args;
      int err = execvp(*arguments, arguments);
      if(err < 0){
	pipeline_free(built_pipeline);
	perror("ERROR: with system call ");
	exit(EXIT_FAILURE);
      }
      pipeline_free(built_pipeline);
      exit(EXIT_SUCCESS);
    }

    //parent process
    else if(cpid > 0){
      pipeline_free(built_pipeline);
      pid_t pid;
      //closing pipes in parent as well
      for(int j = 0; j < process_total - 1; j++){
	close(pipefd[j][1]);
	close(pipefd[j][0]);
      }
      //checking how I want to do the waitpid, background or foreground. Foreground handler just does a return, while background has a waitpid for the entire command line.
      //foreground waits for ALL children before exiting.
      if(built_pipeline -> is_background){
	background_action.sa_handler = background_handler;
	background_action.sa_flags = SA_RESTART; 
	sigaction(SIGCHLD, &background_action, NULL);
      }else{
	foreground_action.sa_handler = foreground_handler;
	sigaction(SIGCHLD, &foreground_action, NULL);
	for(int j = 0; j < process_total; j++){
	  pid = waitpid(-1, &status, 0);
	  if(pid == -1){
	    perror("ERROR: unsuccesful waitpid ");
	    exit(EXIT_FAILURE);
	  }
	}
      }
    }

    else{
      printf("ERROR: with fork ");
      exit(0);
    }
    //again, syorressing prompt
    if(!(argc > 1 && strcmp(argv[1], "-n") == 0)){
      printf("my_shell$ ");
    }
  }
}

void foreground_handler(int signal){
  return;
}

void background_handler(int signal){
  pid_t pid;
  pid = waitpid(-1, 0, 0);
  if(pid == -1){
    perror("ERROR: unsuccesful waitpid within background handler ");
    exit(EXIT_FAILURE);
  }
}
