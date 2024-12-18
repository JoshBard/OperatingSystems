# EC440: Simple Shell
By: Josh Bardwick

To create the simple shell, I began by using the parser that I created for assignment zero. This shell uses fork() and execvp() to create children process and execute commands. Execvp was chosen because the structure of the parser, e.g. a char* array, made it quite simple to use. Additionally the shell can run bakcground processes that are marked by &. This was implemented with the use of two sigaction structures, one for foreground processes and one for background processes. The correct sigaction was chosen by checking the boolean for a background process created in the parser. Additionally CTRL-d was implemented as the exit function. My shell also includes "exit" as a command that exits. Pipes and redirects were implemented with the use of opens, closes, and dup2s, all changing the file descriptor tables of the children processes.

Many pages were used from https://man7.org/linux/man-pages. Additionally code and design from the following textbook was used: https://openosorg.github.io/openos/textbook/scheduling/process.html. The last source of information was lecture notes from EC440. Shout out to my friend and peer Hilario Gonzalez for helping me reason through a lot the places where I got confused by the complicated logic behind the shell. Also the TAs Shamir and Jazmyn helped me out quite a bit.