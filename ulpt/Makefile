override CFLAGS := -Wall -Werror -std=gnu99 -O0 -g $(CFLAGS) -I.

# you can uncomment following to add sanitizer for development; note make sure to not have this for submissions
#override CFLAGS :=  -fsanitize=undefined $(CFLAGS) 
#override LDFLAGS := -fsanitize=undefined -fsanitize=leak $(LDLAGS)  

# Add any additional tests here
test_files= ./test_busy_threads

all: check

# You might find it useful to write test programs and get them working against the installed pthreads
# to do so, set TSTMYPTHREADS to a value other than 1
TSTMYPTHREADS=1
ifeq ($(TSTMYPTHREADS),1)
  mythread=threads.o
else
  mythread=
  override LDFLAGS += -pthread
endif

CC = gcc

# Build the threads.o file
threads.o: threads.c ec440threads.h

# rules to build each of the tests
test_busy_threads.o : test_busy_threads.c 
test_busy_threads: test_busy_threads.o $(mythread)
/*test_pthread_create.o : test_pthread_create.c
test_pthread_create: test_pthread_create.o $(mythread)
test_scheduler_no_clock.o : test_scheduler_no_clock.c
test_scheduler_no_clock: test_scheduler_no_clock.o $(mythread)
test_pthread_join.o : test_pthread_join.c
test_pthread_join: test_pthread_join.o $(mythread)
test_pthread_join2.o : test_pthread_join2.c
test_pthread_join2: test_pthread_join2.o $(mythread)
test_exit_from_main.o : test_exit_from_main.c
test_exit_from_main: test_exit_from_main.o $(mythread)*/

.PHONY: clean check checkprogs

# Build all of the test programs
checkprogs: $(test_files)

# Run the test programs
check: checkprogs
	/bin/bash run_tests.sh $(test_files)

clean:
	rm -f *.o $(test_files) $(test_o_files)
