override CFLAGS := -Wall -Werror -std=gnu99 -pedantic -O0 -g -pthread $(CFLAGS)
override LDLIBS := -pthread $(LDLIBS)

test_files= ./test_create ./test_destroy ./test_edge_cases ./test_read ./test_write_read ./test_write ./test_clone ./test_one_cow ./test_many_cow ./test_clone_large ./test_handler ./test_clone_read ./test_clone_operations ./test_clone_robust

all: check

tls.o: tls.c tls.h

test_create.o : test_create.c tls.h
test_create: test_create.o tls.o

test_destroy.o : test_destroy.c tls.h
test_destroy: test_destroy.o tls.o

test_edge_cases.o : test_edge_cases.c tls.h
test_edge_cases: test_edge_cases.o tls.o

test_read.o : test_read.c tls.h
test_read: test_read.o tls.o

test_write_read.o : test_write_read.c tls.h
test_write_read: test_write_read.o tls.o

test_write.o : test_write.c tls.h
test_write: test_write.o tls.o

test_clone.o : test_clone.c tls.h
test_clone: test_clone.o tls.o

test_clone_read.o : test_clone_read.c tls.h
test_clone_read: test_clone_read.o tls.o

test_one_cow.o : test_one_cow.c tls.h
test_one_cow: test_one_cow.o tls.o

test_many_cow.o : test_many_cow.c tls.h
test_many_cow: test_many_cow.o tls.o

test_clone_large.o : test_clone_large.c tls.h
test_clone_large: test_clone_large.o tls.o

test_handler.o : test_handler.c tls.h
test_handler: test_handler.o tls.o

test_clone_read.o : test_clone_read.c tls.h
test_clone_read: test_clone_read.o tls.o

test_clone_operations.o : test_clone_operations.c tls.h
test_clone_operations: test_clone_operations.o tls.o

test_clone_robust.o : test_clone_robust.c tls.h
test_clone_robust: test_clone_robust.o tls.o

.PHONY: clean check checkprogs

# Build all of the test programs
checkprogs: $(test_files)

# Run the test programs
check: checkprogs
	/bin/bash run_tests.sh $(test_files)

clean:
	rm -f *.o $(test_files) $(test_o_files)

valgrind: checkprogs
	valgrind --leak-check=full --show-leak-kinds=all ./test_create
	valgrind --leak-check=full --show-leak-kinds=all ./test_destroy
	valgrind --leak-check=full --show-leak-kinds=all ./test_edge_cases
	valgrind --leak-check=full --show-leak-kinds=all ./test_read
	valgrind --leak-check=full --show-leak-kinds=all ./test_write_read
	valgrind --leak-check=full --show-leak-kinds=all ./test_write
	valgrind --leak-check=full --show-leak-kinds=all ./test_clone
	valgrind --leak-check=full --show-leak-kinds=all ./test_one_cow
	valgrind --leak-check=full --show-leak-kinds=all ./test_many_cow
	valgrind --leak-check=full --show-leak-kinds=all ./test_clone_large
	valgrind --leak-check=full --show-leak-kinds=all ./test_clone_read
	valgrind --leak-check=full --show-leak-kinds=all ./test_clone_operations

