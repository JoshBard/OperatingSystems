override CFLAGS := -std=gnu99 -O0 -Wall -Werror -g -fsanitize=undefined $(CFLAGS) -I.
override LDFLAGS := -fsanitize=undefined -fsanitize=leak $(LDLAGS)
CC = gcc

# I generally make the first rule run all the tests
all: myshell

# rule for making the parser.o  that is needed by all the test programs
myshell_parser.o: myshell_parser.c myshell_parser.h
myshell.o: myshell.c myshell.h myshell_parser.h

myshell : myshell.o myshell_parser.o

run_files = ./myshell

.PHONY: clean check checkprogs all

# Build all of the test program
checkprogs: $(run_files)

run: myshell ./myshell

check: checkprogs
	/bin/bash run_tests.sh $(run_files)

clean:
	rm -f *~ *.o $(run_files) $(run_o_files)
