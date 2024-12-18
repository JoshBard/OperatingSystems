override CFLAGS := -Wall -Werror -std=gnu99 -pedantic -O0 -g -pthread $(CFLAGS)
override LDLIBS := -pthread -lm $(LDLIBS)

test_files= ./test_make_fs ./test_mount_umount ./test_open_close ./test_fs_delete ./test_fs_create ./test_fs_read ./test_fs_write ./test_get_filesize ./test_big_writes ./test_listfiles ./test_truncate
#./test_bonus

all: check

fs.o: fs.c fs.h disk.h disk.c
disk.o: disk.h disk.c

test_big_writes.o : test_big_writes.c fs.h disk.h
test_big_writes: test_big_writes.o fs.o disk.o

test_bonus.o : test_bonus.c fs.h disk.h
test_bonus: test_bonus.o fs.o disk.o

test_fs_create.o : test_fs_create.c fs.h disk.h
test_fs_create: test_fs_create.o fs.o disk.o

test_fs_delete.o : test_fs_delete.c fs.h disk.h
test_fs_delete: test_fs_delete.o fs.o disk.o

test_fs_read.o : test_fs_read.c fs.h disk.h
test_fs_read: test_fs_read.o fs.o disk.o

test_fs_write.o : test_fs_write.c fs.h disk.h
test_fs_write: test_fs_write.o fs.o disk.o

test_get_filesize.o : test_get_filesize.c fs.h disk.h
test_get_filesize: test_get_filesize.o fs.o disk.o

test_listfiles.o : test_listfiles.c fs.h disk.h
test_listfiles: test_listfiles.o fs.o disk.o

test_make_fs.o : test_make_fs.c fs.h disk.h
test_make_fs: test_make_fs.o fs.o disk.o

test_mount_umount.o : test_mount_umount.c fs.h disk.h
test_mount_umount: test_mount_umount.o fs.o disk.o

test_open_close.o : test_open_close.c fs.h disk.h
test_open_close: test_open_close.o fs.o disk.o

test_truncate.o : test_truncate.c fs.h disk.h
test_truncate: test_truncate.o fs.o disk.o

.PHONY: clean check checkprogs

# Build all of the test programs
checkprogs: $(test_files)

# Run the test programs
check: checkprogs
	/bin/bash run_tests.sh $(test_files)

clean:
	rm -f *.o $(test_files) $(test_o_files)

valgrind: checkprogs
	valgrind --leak-check=full --show-leak-kinds=all ./
