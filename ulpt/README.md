By: Josh Bardwick

This is my implementation of a user-level preemption threading library with a round robin scheduler. This assignment takes advantage of the ualarm and sigaction functions to create an interrupt signal every quantum.
The signal handler was the function schedule, which looks for the next thread that is marked as ready and longjmps to it. To keep track of individual thread information, I created a thread_control_block struct, and
to maintain the queue of threads I created a runqueue struct with a FIRST, CURRENT, and LAST member. In the thread_control_block, the current context is maintained with a set_jmp(jmp_buf) and we switch context with
longjmp(). 

For references I used: https://man7.org/linux/man-pages/index.html as well as helps from TAs in my EC440 course.
